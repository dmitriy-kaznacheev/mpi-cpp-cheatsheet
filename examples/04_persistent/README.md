# 04_persistent

Кольцевой сдвиг в цикле на persistent-запросах:
`MPI_Send_init`/`MPI_Recv_init` один раз, `MPI_Startall`/`MPI_Waitall`
на каждой итерации.

## Сборка и запуск

Из корня репозитория:

    cmake -S . -B build
    cmake --build build -j
    mpirun -np 4 ./build/bin/persistent 5 --verify

Первый аргумент — число итераций (по умолчанию 5).
`--verify` проверяет последний принятый элемент.

## Ожидаемый вывод с `--verify`

    OK (persistent ring, 5 iters)

## Что демонстрирует

- `MPI_Send_init`/`MPI_Recv_init` — создание persistent-запросов.
- `MPI_Startall`/`MPI_Waitall` — запуск и ожидание пары запросов в цикле.
- `MPI_Request_free` — освобождение persistent-запросов после цикла.
- Экономия: параметры обмена (`dest`, `count`, `tag`) фиксируются один раз.

## Замечания

- Буфер (`send_val`/`recv_val`) привязан к запросу при `*_init`. Менять
  его **содержимое** между итерациями можно, менять **адрес** — нет.
- `MPI_Wait`/`MPI_Waitall` **не** освобождают persistent-запрос —
  нужен `MPI_Request_free`.
- Для одной итерации persistent дороже обычного `Isend`/`Irecv`:
  выгода появляется при `n_iters >> 1`.

## См. также

- [`docs/04-nonblocking.md`](../../docs/04-nonblocking.md) — `Isend`/`Irecv`, `Wait`/`Test`
- [`docs/05-persistent.md`](../../docs/05-persistent.md) — теория persistent-запросов