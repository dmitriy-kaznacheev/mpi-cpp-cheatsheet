# 02_matvec

Умножение матрицы на вектор `y = A * x` с распределением строк между процессами.
Две реализации: на point-to-point и на коллективных операциях.

## Сборка и запуск

Из корня репозитория:

    cmake -S . -B build
    cmake --build build -j

Версия на `MPI_Send`/`MPI_Recv`:

    mpirun -np 4 ./build/bin/matvec_send 400 --verify

Версия на `MPI_Bcast`/`MPI_Scatterv`/`MPI_Gatherv`:

    mpirun -np 4 ./build/bin/matvec 400 --verify

Первый аргумент — размер `N` (по умолчанию 400). Флаг `--verify` включает
сравнение с последовательным вычислением и печатает `OK` / `FAIL`.

## Ожидаемый вывод с `--verify`

    OK (max diff = 0)

## Что демонстрирует

- Распределение строк: процесс `rank` получает `N/size` строк
  (первые `N % size` процессов — на одну больше).
- `matvec_send`: ручная рассылка `x` и `A` через `MPI_Send`/`MPI_Recv`,
  ручной сбор `y`, теги `TAG_X`/`TAG_A`/`TAG_Y`.
- `matvec`: `MPI_Bcast` для `x`, `MPI_Scatterv` для `A`, `MPI_Gatherv` для `y`.
- Общий код в [`examples/common/common.hpp`](../common/common.hpp):
  `MpiEnvironment`, парсер аргументов, `block_for`, `seq_matvec`.

## Замечания

- `N` ограничен ~46340: `count` в `MPI_Send`/`MPI_Scatterv` — `int`,
  и `N * N` должно влезать в `int`. Проверка — `check_size_limit`.
- Порядок `Send`/`Recv` на корне и получателях должен совпадать,
  иначе дедлок (см. `docs/02-point-to-point.md`).
- `matvec` — учебная замена ручных циклов; в реальном коде `Scatterv`/`Gatherv`
  предпочтительнее `Send`/`Recv` из-за перекрытия и простоты.

## См. также

- [`docs/02-point-to-point.md`](../../docs/02-point-to-point.md) — `MPI_Send`/`MPI_Recv`, джокеры, `MPI_Sendrecv`
- [`docs/03-collective.md`](../../docs/03-collective.md) — `Bcast`/`Scatterv`/`Gatherv`