# 03 — Коллективные операции

Коллективные операции — это обмен, в котором участвуют **все** процессы
коммуникатора. Они сами заботятся о порядке, перекрытии и балансировке;
ручные циклы `Send`/`Recv` нужны только в нестандартных протоколах.

## Основные операции

| Функция | Что делает | Кто получает результат |
|---|---|---|
| `MPI_Bcast` | Рассылает буфер от root всем | все |
| `MPI_Scatter` | Раздаёт куски буфера от root всем | все (по куску) |
| `MPI_Gather` | Собирает куски со всех на root | только root |
| `MPI_Allgather` | Собирает куски со всех, результат у всех | все |
| `MPI_Reduce` | Свёртка (сумма, max, …) на root | только root |
| `MPI_Allreduce` | Свёртка, результат у всех | все |
| `MPI_Barrier` | Барьер синхронизации | — |

## `MPI_Bcast`

```cpp
int MPI_Bcast(void* buffer, int count, MPI_Datatype datatype,
              int root, MPI_Comm comm);
```

- На `root` буфер содержит данные, которые надо разослать.
- На остальных буфер **того же размера**, куда данные будут приняты.
- После вызова буфер корректен у всех.

Заменяет цикл:

```cpp
// было (шаг 3):
if (rank == 0) {
    for (int dest = 1; dest < size; ++dest)
        MPI_Send(x.data(), N, MPI_DOUBLE, dest, TAG_X, MPI_COMM_WORLD);
} else {
    MPI_Recv(x.data(), N, MPI_DOUBLE, 0, TAG_X, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

// стало:
MPI_Bcast(x.data(), N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
```

## `MPI_Scatter`

```cpp
int MPI_Scatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype,
                void* recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm);
```

- `sendbuf` значащ только на `root`: там лежат `size * sendcount` элементов,
  разбитых на блоки по `sendcount`.
- `recvbuf` у всех: туда приходит блок процесса `rank`.
- `sendcount` и `recvcount` обычно равны; если блоки разного размера —
  нужен `MPI_Scatterv`.

Заменяет цикл:

```cpp
// было (шаг 3):
if (rank == 0) {
    for (int dest = 1; dest < size; ++dest) {
        auto [off, rows] = block_for(dest, N, size);
        MPI_Send(&A[off * N], rows * N, MPI_DOUBLE, dest, TAG_A, MPI_COMM_WORLD);
    }
    std::copy(...);  // корень копирует свой блок
} else {
    MPI_Recv(local_A.data(), local_rows * N, MPI_DOUBLE, 0, TAG_A, ...);
}

// стало (если блоки одинакового размера):
MPI_Scatter(A.data(), local_rows * N, MPI_DOUBLE,
            local_A.data(), local_rows * N, MPI_DOUBLE,
            0, MPI_COMM_WORLD);
```

## `MPI_Scatterv` — блоки разного размера

Если `N % size != 0`, блоки разной длины. Тогда:

```cpp
int MPI_Scatterv(const void* sendbuf, const int* sendcounts, const int* displs,
                 MPI_Datatype sendtype,
                 void* recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm);
```

- `sendcounts[i]` — сколько элементов отдать процессу `i`.
- `displs[i]` — смещение блока процесса `i` в `sendbuf` (в элементах).
- `recvcount` — сколько примет **этот** процесс (у каждого своё).

Для `02_matvec` именно `Scatterv` подходит, потому что `local_rows` разный.

## `MPI_Gather` / `MPI_Gatherv`

Симметрично `Scatter`/`Scatterv`: собирает куски со всех на root.
`Gatherv` — когда куски разного размера (нужны `recvcounts` и `displs`).

## `MPI_Reduce` / `MPI_Allreduce`

```cpp
int MPI_Reduce(const void* sendbuf, void* recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
```

- `sendbuf` — локальное значение у каждого.
- `recvbuf` — результат свёртки, значащ только на `root`.
- `op`: `MPI_SUM`, `MPI_MAX`, `MPI_MIN`, `MPI_PROD`, `MPI_LAND`, …

`MPI_Allreduce` — то же, но результат у всех (полезно для согласования
статуса `OK`/`FAIL` после `--verify`).

## Барьер

```cpp
MPI_Barrier(MPI_COMM_WORLD);
```

Блокируется, пока все процессы не вызовут. Нужен редко — для синхронизации
вывода или замеров времени. В вычислительных циклах обычно не нужен:
коллективные операции сами синхронизируют.

## Типичные ошибки

| Ошибка | Последствие |
|---|---|
| Разный `count`/`datatype` у процессов в `Bcast` | Неверные данные / переполнение |
| `root` не совпадает у отправителя и получателей | Дедлок или неверный результат |
| `Scatter` с блоками разного размера без `Scatterv` | Неверное разбиение / потеря данных |
| `Reduce` без `MPI_Allreduce`, когда результат нужен всем | На не-root `recvbuf` — мусор |
| `MPI_Barrier` в ветке `if (rank == 0)` | Дедлок: остальные до барьера не дойдут |