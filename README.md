# MPI C++ Cheatsheet

Личная шпаргалка по MPI на C++: краткая теория, рабочие примеры,
подводные камни. 

## Быстрый старт

    cmake -S . -B build
    cmake --build build -j
    mpirun -np 4 ./build/bin/hello

## Документация

- [00 — Установка и окружение](docs/00-setup.md)
- [01 — Hello world](docs/01-hello-world.md)
- [02 — Point-to-point](docs/02-point-to-point.md)
- [03 — Коллективные операции](docs/03-collective.md)
- [04 — Неблокирующие операции](docs/04-nonblocking.md)
- [05 — Persistent-запросы](docs/05-persistent.md)
- [06 — Топологии](docs/06-topologies.md)
- [07 — Коммуникаторы и группы](docs/07-communicators.md)
- [08 — Подводные камни](docs/08-pitfalls.md)

## Примеры

- `01_hello` — минимальная MPI-программа
- `02_matvec` — умножение матрицы на вектор:
  - `matvec_send` — на `MPI_Send`/`MPI_Recv` (point-to-point)
  - `matvec` — на `MPI_Bcast`/`MPI_Scatterv`/`MPI_Gatherv` (коллективные)
- `03_ring` — кольцевой сдвиг на `MPI_Isend`/`MPI_Irecv`
- `examples/common/` — общий код (`MpiEnvironment`, парсер аргументов, `block_for`)

Планируется: `04_cg` (сопряжённые градиенты), `05_tridiag` (прогонка).

## Сборка

- CMake ≥ 3.16
- Компилятор с поддержкой C++17
- MPI (OpenMPI или MPICH), `find_package(MPI)` должен находить `MPI::MPI_CXX`