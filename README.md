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

- `01_hello` — минимальная программа
- `02_matvec` — умножение матрицы на вектор
- `03_cg` — метод сопряжённых градиентов
- `04_tridiag` — метод прогонки

## Сборка

- CMake ≥ 3.16
- Компилятор с поддержкой C++17
- MPI (OpenMPI или MPICH), `find_package(MPI)` должен находить `MPI::MPI_CXX`