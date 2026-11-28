#include "common.hpp"

using namespace mpi_cheatsheet;

// Кольцевой сдвиг: каждый отправляет свой rank соседу справа,
// принимает от соседа слева. С Isend/Irecv дедлока нет.
void ring_shift(int rank, int size, int &recv_val) {
  int send_val = rank;
  recv_val = -1;

  int right = (rank + 1) % size;
  int left = (rank - 1 + size) % size;

  MPI_Request reqs[2];
  MPI_Isend(&send_val, 1, MPI_INT, right, 0, MPI_COMM_WORLD, &reqs[0]);
  MPI_Irecv(&recv_val, 1, MPI_INT, left, 0, MPI_COMM_WORLD, &reqs[1]);
  MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
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
    (void)N;
    (void)iters;

    int recv_val = -1;
    ring_shift(rank, size, recv_val);

    if (verify) {
      int expected = (rank - 1 + size) % size;
      if (recv_val != expected) {
        std::cerr << "rank " << rank << ": expected " << expected << ", got "
                  << recv_val << "\n";
        return EXIT_FAILURE;
      }
      if (rank == 0) {
        std::cout << "OK (ring shift correct)\n";
      }
    } else {
      std::cout << "rank " << rank << " received " << recv_val << "\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}