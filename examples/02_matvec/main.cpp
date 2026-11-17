#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <numeric>
#include <optional>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace {

// RAII для MPI
class MpiEnvironment {
public:
  MpiEnvironment(int &argc, char **&argv) {
    int initialized = 0;
    MPI_Initialized(&initialized);
    if (!initialized) {
      if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        throw std::runtime_error("Ошибка инициализации MPI");
      }
    }
  }

  ~MpiEnvironment() {
    int finalized = 0;
    MPI_Finalized(&finalized);
    if (!finalized) {
      MPI_Finalize();
    }
  }

  MpiEnvironment(const MpiEnvironment &) = delete;
  MpiEnvironment &operator=(const MpiEnvironment &) = delete;
  MpiEnvironment(MpiEnvironment &&) = delete;
  MpiEnvironment &operator=(MpiEnvironment &&) = delete;
};

// Разбор аргументов

struct CliArgs {
  int count = 400;
  bool verify = false;
};

std::optional<CliArgs> par_parse_args(int argc, char **argv, int rank) {
  // TODO очень простой парсер, возможно нужно улучшить
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    if (std::string_view(argv[i]) == "--verify"sv) {
      args.verify = true;
    } else {
      std::string_view arg{argv[i]};
      int value = 0;
      auto [ptr, ec] =
          std::from_chars(arg.data(), arg.data() + arg.size(), value);
      if (ec != std::errc{} || ptr != arg.data() + arg.size()) {
        if (rank == 0)
          std::cerr << "Некорректный аргумент: " << arg << "\n";
        return std::nullopt;
      }
      args.count = value;
    }
  }

  if (args.count <= 0) {
    if (rank == 0) {
      std::cerr << "Некорректный размер\n";
    }
    return std::nullopt;
  }

  return args;
}

} // namespace

namespace helper {

enum Tag : int { TAG_X = 0, TAG_A = 1, TAG_Y = 2 };

struct Block {
  int offset;
  int rows;
};

Block block_for(int rank, int N, int size) {
  // распределение строк (первые remainder процессов получают на 1 больше)
  int base = N / size;
  int rem = N % size;
  int rows = base + (rank < rem ? 1 : 0);
  int off = rank * base + std::min(rank, rem);
  return {off, rows};
}

} // namespace helper

// последовательное умножение для проверки
void seq_matvec(int N, const std::vector<double> &A,
                const std::vector<double> &x, std::vector<double> &y_out) {
  for (int i = 0; i < N; ++i) {
    double sum = 0.0;
    for (int j = 0; j < N; ++j) {
      sum += A[i * N + j] * x[j];
    }
    y_out[i] = sum;
  }
}

// параллельное умножение
void par_matvec(int N, int rank, int size, const std::vector<double> &A,
                std::vector<double> &x, std::vector<double> &y_out) {
  auto [offset, local_rows] = helper::block_for(rank, N, size);

  std::vector<double> local_y(local_rows);
  std::vector<double> local_A(local_rows * N);

  // рассылка вектора x всем процессам
  if (rank == 0) {
    for (int dest = 1; dest < size; ++dest) {
      MPI_Send(x.data(), N, MPI_DOUBLE, dest, helper::TAG_X, MPI_COMM_WORLD);
    }
  } else {
    MPI_Recv(x.data(), N, MPI_DOUBLE, 0, helper::TAG_X, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  }

  // рассылка локальных блоков матрицы
  if (rank == 0) {
    for (int dest = 1; dest < size; ++dest) {
      auto [d_off, d_rows] = helper::block_for(dest, N, size);
      MPI_Send(&A[d_off * N], d_rows * N, MPI_DOUBLE, dest, helper::TAG_A,
               MPI_COMM_WORLD);
    }
    // корень копирует свой блок
    std::copy(&A[offset * N], &A[(offset + local_rows) * N], local_A.begin());
  } else {
    MPI_Recv(local_A.data(), local_rows * N, MPI_DOUBLE, 0, helper::TAG_A,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  // локальное вычисление
  for (int i = 0; i < local_rows; ++i) {
    double sum = 0.0;
    for (int j = 0; j < N; ++j) {
      sum += local_A[i * N + j] * x[j];
    }
    local_y[i] = sum;
  }

  // сбор результатов на корне
  if (rank == 0) {
    std::copy(local_y.begin(), local_y.end(), y_out.begin() + offset);
    for (int src = 1; src < size; ++src) {
      auto [s_off, s_rows] = helper::block_for(src, N, size);
      MPI_Recv(&y_out[s_off], s_rows, MPI_DOUBLE, src, helper::TAG_Y,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
  } else {
    MPI_Send(local_y.data(), local_rows, MPI_DOUBLE, 0, helper::TAG_Y,
             MPI_COMM_WORLD);
  }
}

int main(int argc, char **argv) {
  try {
    MpiEnvironment mpi_env{argc, argv};

    int rank{0}, size{0};
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    auto args = par_parse_args(argc, argv, rank);
    if (!args) {
      return EXIT_FAILURE;
    }

    auto [N, verify] = *args;

    std::vector<double> A;    // только на rank 0
    std::vector<double> y;    // полный результат (на rank 0)
    std::vector<double> x(N); // у всех после рассылки

    // инициализация на корне
    if (rank == 0) {
      A.resize(N * N);
      y.resize(N);
      for (int i = 0; i < N * N; ++i) {
        A[i] = 1.0 + (i % 10) * 0.1;
      }
      for (int j = 0; j < N; ++j) {
        x[j] = 1.0 + j * 0.01;
      }
    }

    // параллельный расчет
    par_matvec(N, rank, size, A, x, y);

    // проверка (только на корне)
    if (verify && rank == 0) {
      std::vector<double> y_seq(N);
      seq_matvec(N, A, x, y_seq);

      double max_diff = 0.0;
      for (int i = 0; i < N; ++i) {
        max_diff = std::max(max_diff, std::fabs(y[i] - y_seq[i]));
      }
      if (max_diff < 1e-10) {
        std::cout << "OK (max diff = " << max_diff << ")\n";
      } else {
        std::cout << "FAIL (max diff = " << max_diff << ")\n";
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}