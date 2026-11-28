#pragma once

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <mpi.h>
#include <optional>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

namespace mpi_cheatsheet {

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
    if (!finalized)
      MPI_Finalize();
  }
  MpiEnvironment(const MpiEnvironment &) = delete;
  MpiEnvironment &operator=(const MpiEnvironment &) = delete;
  MpiEnvironment(MpiEnvironment &&) = delete;
  MpiEnvironment &operator=(MpiEnvironment &&) = delete;
};

struct CliArgs {
  int count = 400; // размер задачи (N) — для 02_matvec
  int iters = 5;   // число итераций — для 04_persistent
  bool verify = false;
};

// Разбор одного целого. nullopt при ошибке.
inline std::optional<int> parse_int(std::string_view arg, int rank,
                                    const char *what) {
  int value = 0;
  auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
  if (ec != std::errc{} || ptr != arg.data() + arg.size()) {
    if (rank == 0) {
      std::cerr << "Некорректный " << what << ": " << arg << "\n";
    }
    return std::nullopt;
  }
  return value;
}

// Общий парсер: --verify, --iters N, позиционное число -> count.
inline std::optional<CliArgs> par_parse_args(int argc, char **argv, int rank) {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg{argv[i]};
    if (arg == "--verify"sv) {
      args.verify = true;
    } else if (arg == "--iters"sv) {
      if (i + 1 >= argc) {
        if (rank == 0) {
          std::cerr << "--iters требует значение\n";
        }
        return std::nullopt;
      }
      auto v = parse_int(argv[++i], rank, "iters");
      if (!v) {
        return std::nullopt;
      }
      args.iters = *v;
      if (args.iters < 1) {
        if (rank == 0) {
          std::cerr << "iters должен быть >= 1\n";
        }
        return std::nullopt;
      }
    } else {
      auto v = parse_int(arg, rank, "аргумент");
      if (!v) {
        return std::nullopt;
      }
      args.count = *v;
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

struct Block {
  int offset;
  int rows;
};

inline Block block_for(int rank, int N, int size) {
  int base = N / size;
  int rem = N % size;
  int rows = base + (rank < rem ? 1 : 0);
  int off = rank * base + std::min(rank, rem);
  return {off, rows};
}

inline void seq_matvec(int N, const std::vector<double> &A,
                       const std::vector<double> &x,
                       std::vector<double> &y_out) {
  for (int i = 0; i < N; ++i) {
    double sum = 0.0;
    for (int j = 0; j < N; ++j) {
      sum += A[i * N + j] * x[j];
    }
    y_out[i] = sum;
  }
}

// Общая проверка размера задачи (count в MPI — int)
inline bool check_size_limit(int N, int rank) {
  int ok = 1;
  if (rank == 0) {
    long long max_count = static_cast<long long>(N) * N;
    if (max_count > std::numeric_limits<int>::max()) {
      std::cerr << "N too large for single collective/Send (max ~46340)\n";
      ok = 0;
    }
  }
  MPI_Bcast(&ok, 1, MPI_INT, 0, MPI_COMM_WORLD);
  return ok == 1;
}

// Общая инициализация A, x, y на корне
inline void init_root_data(int N, int rank, std::vector<double> &A,
                           std::vector<double> &x, std::vector<double> &y) {
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
}

} // namespace mpi_cheatsheet