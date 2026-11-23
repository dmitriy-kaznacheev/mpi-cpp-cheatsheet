#include "common.hpp"

using namespace mpi_cheatsheet;

namespace {
enum Tag : int { TAG_X = 0, TAG_A = 1, TAG_Y = 2 };
} // namespace

void par_matvec(int N, int rank, int size, const std::vector<double> &A,
                std::vector<double> &x, std::vector<double> &y_out) {
  auto [offset, local_rows] = block_for(rank, N, size);

  std::vector<double> local_y(local_rows);
  std::vector<double> local_A(local_rows * N);

  // рассылка x
  if (rank == 0) {
    for (int dest = 1; dest < size; ++dest) {
      MPI_Send(x.data(), N, MPI_DOUBLE, dest, TAG_X, MPI_COMM_WORLD);
    }
  } else {
    MPI_Recv(x.data(), N, MPI_DOUBLE, 0, TAG_X, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
  }

  // рассылка A
  if (rank == 0) {
    for (int dest = 1; dest < size; ++dest) {
      auto [d_off, d_rows] = block_for(dest, N, size);
      MPI_Send(&A[d_off * N], d_rows * N, MPI_DOUBLE, dest,
               TAG_A, MPI_COMM_WORLD);
    }
    std::copy(&A[offset * N], &A[(offset + local_rows) * N], local_A.begin());
  } else {
    MPI_Recv(local_A.data(), local_rows * N, MPI_DOUBLE, 0,
             TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }

  // вычисление
  for (int i = 0; i < local_rows; ++i) {
    double sum = 0.0;
    for (int j = 0; j < N; ++j) {
      sum += local_A[i * N + j] * x[j];
    }
    local_y[i] = sum;
  }

  // сбор y
  if (rank == 0) {
    std::copy(local_y.begin(), local_y.end(), y_out.begin() + offset);
    for (int src = 1; src < size; ++src) {
      auto [s_off, s_rows] = block_for(src, N, size);
      MPI_Recv(&y_out[s_off], s_rows, MPI_DOUBLE, src, TAG_Y,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
  } else {
    MPI_Send(local_y.data(), local_rows, MPI_DOUBLE, 0, TAG_Y,
             MPI_COMM_WORLD);
  }
}

int main(int argc, char **argv) {
  try {
    MpiEnvironment mpi_env{argc, argv};

    int rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    auto args = par_parse_args(argc, argv, rank);
    if (!args) {
      return EXIT_FAILURE;
    }

    auto [N, verify] = *args;

    if (!check_size_limit(N, rank)) {
      return EXIT_FAILURE;
    }

    std::vector<double> A, y, x(N);
    init_root_data(N, rank, A, x, y);

    par_matvec(N, rank, size, A, x, y);

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
        return EXIT_FAILURE;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}