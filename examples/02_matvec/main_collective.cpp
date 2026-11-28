#include "common.hpp"

using namespace mpi_cheatsheet;

void par_matvec(int N, int rank, int size, const std::vector<double> &A,
                std::vector<double> &x, std::vector<double> &y_out) {
  auto [offset, local_rows] = block_for(rank, N, size);

  // рассылка x
  MPI_Bcast(x.data(), N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  // scatterv для A
  std::vector<int> sendcounts, displs;
  if (rank == 0) {
    sendcounts.resize(size);
    displs.resize(size);
    for (int i = 0; i < size; ++i) {
      auto [off, rows] = block_for(i, N, size);
      sendcounts[i] = rows * N;
      displs[i] = off * N;
    }
  }
  std::vector<double> local_A(local_rows * N);
  MPI_Scatterv(rank == 0 ? A.data() : nullptr,
               rank == 0 ? sendcounts.data() : nullptr,
               rank == 0 ? displs.data() : nullptr, MPI_DOUBLE, local_A.data(),
               (local_rows * N), MPI_DOUBLE, 0, MPI_COMM_WORLD);

  // локальное вычисление
  std::vector<double> local_y(local_rows);
  for (int i = 0; i < local_rows; ++i) {
    double sum = 0.0;
    for (int j = 0; j < N; ++j) {
      sum += local_A[i * N + j] * x[j];
    }
    local_y[i] = sum;
  }

  // gatherv для y
  std::vector<int> recvcounts, rdispls;
  if (rank == 0) {
    recvcounts.resize(size);
    rdispls.resize(size);
    for (int i = 0; i < size; ++i) {
      auto [off, rows] = block_for(i, N, size);
      recvcounts[i] = rows;
      rdispls[i] = off;
    }
  }
  MPI_Gatherv(local_y.data(), local_rows, MPI_DOUBLE,
              rank == 0 ? y_out.data() : nullptr,
              rank == 0 ? recvcounts.data() : nullptr,
              rank == 0 ? rdispls.data() : nullptr, MPI_DOUBLE, 0,
              MPI_COMM_WORLD);
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

    auto [N, iters, verify] = *args;
    (void)iters;

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