# 02 — Point-to-point

## Две основные функции

```cpp
int MPI_Send(const void* buf, int count, MPI_Datatype datatype,
             int dest, int tag, MPI_Comm comm);

int MPI_Recv(void* buf, int count, MPI_Datatype datatype,
             int source, int tag, MPI_Comm comm, MPI_Status* status);
```

- **`MPI_Send`** — блокирующая отправка. После возврата из функции буфер `buf` можно использовать повторно (MPI скопировал данные или отправил их).
- **`MPI_Recv`** — блокирующий приём. После возврата сообщение уже в `buf`. Если в очереди нет подходящего сообщения, процесс **блокируется** до его прихода.

## Параметры

| Параметр | Смысл |
|---|---|
| `buf` | Адрес начала буфера |
| `count` | Число элементов (не байт!) |
| `datatype` | MPI-тип: `MPI_INT`, `MPI_DOUBLE`, `MPI_CHAR`, … |
| `dest` / `source` | Ранг процесса-получателя / отправителя |
| `tag` | Метка сообщения (целое, 0..`MPI_TAG_UB`) |
| `comm` | Коммуникатор |
| `status` | Заполняется при приёме: кто отправил, с каким тегом, сколько элементов |

## Джокеры

В `MPI_Recv` можно принимать от любого источника и с любым тегом [citation:11]:

```cpp
MPI_Recv(buf, count, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
         MPI_COMM_WORLD, &status);
```

После приёма узнать реальные значения:
```cpp
int src = status.MPI_SOURCE;
int tag = status.MPI_TAG;
int count;
MPI_Get_count(&status, MPI_INT, &count);
```

## Статус

`MPI_Status` — структура, поля которой заполняет `MPI_Recv`. Если статус не нужен, передают `MPI_STATUS_IGNORE`.

## Типичный паттерн master-worker

Процесс 0 раздаёт данные, остальные принимают:

```cpp
if (rank == 0) {
    for (int dest = 1; dest < size; ++dest) {
        MPI_Send(data_for[dest], n, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD);
    }
} else {
    MPI_Recv(my_data, n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
```

## Сбор результатов

Обратный паттерн — workers отправляют результат на root:

```cpp
if (rank == 0) {
    for (int src = 1; src < size; ++src) {
        MPI_Recv(result_for[src], n, MPI_DOUBLE, src, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
} else {
    MPI_Send(my_result, n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
}
```

## `MPI_Sendrecv` — обмен без дедлока

Если два процесса одновременно делают `MPI_Send` друг другу, возникает **дедлок**: оба ждут, пока освободится буфер получателя. Решение — `MPI_Sendrecv`:

```cpp
MPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
             recvbuf, recvcount, recvtype, source, recvtag,
             comm, &status);
```

Функция атомарно выполняет отправку и приём, MPI сам разбирается с порядком операций.

## Типичные ошибки

| Ошибка | Последствие |
|---|---|
| Несовпадение `count`/`datatype` у отправителя и получателя | Неверные данные или переполнение буфера |
| Разные теги у `Send` и `Recv` | `MPI_Recv` блокируется навсегда |
| `MPI_Send` и `MPI_Recv` одновременно у двух процессов | Взаимный дедлок |
| Забыть `MPI_Get_count`, если принимающий буфер больше сообщения | В буфере останется мусор после принятых элементов |
| Использовать `MPI_Send` с буфером, который меняется до завершения | Неопределённое поведение (для стандартного send MPI мог не скопировать) |