# 03_ring

Кольцевой сдвиг: каждый процесс отправляет свой `rank` соседу справа и
принимает от соседа слева. Демонстрирует `MPI_Isend`/`MPI_Irecv`/`MPI_Waitall`.

## Сборка и запуск

Из корня репозитория:

    cmake -S . -B build
    cmake --build build -j
    mpirun -np 4 ./build/bin/ring --verify

Без `--verify` печатает, кто что получил:

    mpirun -np 4 ./build/bin/ring

## Ожидаемый вывод с `--verify`

    OK (ring shift correct)

## Что демонстрирует

- `MPI_Isend`/`MPI_Irecv` — неблокирующие операции, возвращают `MPI_Request`.
- `MPI_Waitall` — ожидание массива запросов.
- Почему блокирующие `Send`/`Recv` в том же порядке дали бы дедлок:
  все процессы ждут получателя, а получатель тоже ждёт.

## Замечания

- Буфер нельзя менять между `Isend` и `Wait` — MPI ещё использует его.
- После `MPI_Wait`/`MPI_Waitall` запрос освобождён, повторно использовать нельзя.
- При `size == 1` сосед справа и слева — сам процесс; `Isend`/`Irecv`
  с самим собой работает, но проверка `expected == rank` всё равно корректна.

## См. также

- [`docs/04-nonblocking.md`](../../docs/04-nonblocking.md) — `Isend`/`Irecv`, `Wait`/`Test`, типичные ошибки