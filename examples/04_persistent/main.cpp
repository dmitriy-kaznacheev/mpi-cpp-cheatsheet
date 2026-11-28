#include "common.hpp"

using namespace mpi_cheatsheet;

// Кольцевой сдвиг на persistent-запросах: Send_init/Recv_init один раз,
// Startall/Waitall в цикле.
void ring_persistent(int rank, int size, int n_iters, int &last_recv) {
  int right = (rank + 1) % size;
  int left = (rank - 1 + size) % size;

  int send_val = 0;
  int recv_val = -1;
  last_recv = -1;

  MPI_Request reqs[2];
  MPI_Send_init(&send_val, 1, MPI_INT, right, 0, MPI_COMM_WORLD, &reqs[0]);
  MPI_Recv_init(&recv_val, 1, MPI_INT, left, 0, MPI_COMM_WORLD, &reqs[1]);

  for (int iter = 0; iter < n_iters; ++iter) {
    send_val = rank * 1000 + iter; // новое значение каждый раз
    MPI_Startall(2, reqs);
    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
    last_recv = recv_val;
  }

  MPI_Request_free(&reqs[0]);
  MPI_Request_free(&reqs[1]);
}

int main(int argc, char **argv) {
  try {
    MpiEnvironment mpi_env{argc, argv};

    int rank = 0, size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Разбор: первый числовой аргумент — n_iters, --verify — проверка
    int n_iters = 5;
    bool verify = false;
    for (int i = 1; i < argc; ++i) {
      std::string_view arg{argv[i]};
      if (arg == "--verify"sv) {
        verify = true;
      } else {
        int value = 0;
        auto [ptr, ec] =
            std::from_chars(arg.data(), arg.data() + arg.size(), value);
        if (ec != std::errc{} || ptr != arg.data() + arg.size() || value < 1) {
          if (rank == 0) {
            std::cerr << "Некорректный аргумент: " << arg << "\n";
          }
          return EXIT_FAILURE;
        }
        n_iters = value;
      }
    }

    int last_recv = -1;
    ring_persistent(rank, size, n_iters, last_recv);

    if (verify) {
      // Ожидаемое: на последней итерации сосед слева отправил
      // (left * 1000 + (n_iters - 1))
      int left = (rank - 1 + size) % size;
      int expected = left * 1000 + (n_iters - 1);
      if (last_recv != expected) {
        std::cerr << "rank " << rank << ": expected " << expected << ", got "
                  << last_recv << "\n";
        return EXIT_FAILURE;
      }
      if (rank == 0) {
        std::cout << "OK (persistent ring, " << n_iters << " iters)\n";
      }
    } else {
      std::cout << "rank " << rank << " last received " << last_recv << "\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}