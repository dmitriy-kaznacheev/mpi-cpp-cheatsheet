# 01_hello

Минимальная MPI-программа: инициализация, ранг, размер, финализация.

## Сборка и запуск

Из корня репозитория:

    cmake -S . -B build
    cmake --build build -j
    mpirun -np 4 ./build/bin/hello

Ожидаемый вывод (порядок строк произвольный):

    Hello from rank 0 of 4
    Hello from rank 1 of 4
    Hello from rank 2 of 4
    Hello from rank 3 of 4

## Что демонстрирует

- `MPI_Init` / `MPI_Finalize` — рамка любой MPI-программы
- `MPI_Comm_rank` — номер процесса в коммуникаторе
- `MPI_Comm_size` — число процессов в коммуникаторе
- `MPI_COMM_WORLD` — коммуникатор по умолчанию

## Замечания

- Порядок вывода не гарантирован: процессы пишут в stdout независимо.
- Если ядер меньше, чем `-np`, добавьте `--oversubscribe`:
  `mpirun --oversubscribe -np 4 ./build/bin/hello`

## См. также

- [`docs/01-hello-world.md`](../../docs/01-hello-world.md) — разбор функций и типичные ошибки