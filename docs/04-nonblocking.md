# 04 — Неблокирующие операции

Блокирующие `MPI_Send`/`MPI_Recv` останавливают процесс до завершения
операции. Неблокирующие `MPI_Isend`/`MPI_Irecv` только **запускают**
операцию и возвращают управление сразу; завершение проверяется отдельно
через `MPI_Wait`/`MPI_Test`.

## Зачем

1. **Перекрытие вычислений и коммуникаций.** Пока данные летят по сети,
   процесс считает. Это главный выигрыш в реальных задачах.
2. **Обмен без дедлока.** Два процесса могут одновременно `Isend` друг
   другу и потом `Wait` — дедлока не будет, потому что `Isend` не ждёт
   получателя.
3. **Пайплайны.** Разбить большое сообщение на куски и отправлять их
   по мере готовности.

## Основные функции

```cpp
int MPI_Isend(const void* buf, int count, MPI_Datatype dt,
              int dest, int tag, MPI_Comm comm, MPI_Request* request);

int MPI_Irecv(void* buf, int count, MPI_Datatype dt,
              int source, int tag, MPI_Comm comm, MPI_Request* request);

int MPI_Wait(MPI_Request* request, MPI_Status* status);
int MPI_Waitall(int count, MPI_Request* requests, MPI_Status* statuses);
int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status);
```

- `MPI_Isend`/`MPI_Irecv` возвращают `MPI_Request` — дескриптор операции.
- `MPI_Wait` блокируется, пока операция не завершится, и освобождает
  `request` (после `Wait` его нельзя использовать).
- `MPI_Waitall` ждёт **все** запросы из массива.
- `MPI_Test` — неблокирующая проверка: `flag = 1`, если завершилось.

## Важно: буфер нельзя трогать до `Wait`

```cpp
MPI_Isend(buf.data(), n, MPI_DOUBLE, dest, tag, comm, &req);
// buf всё ещё используется MPI! Менять нельзя.
// ...
MPI_Wait(&req, MPI_STATUS_IGNORE);
// теперь buf свободен
```

Это главная ловушка: `Isend` **не копирует** буфер (в отличие от
некоторых реализаций блокирующего `Send` для маленьких сообщений).
До `Wait` буфер принадлежит MPI.

## Пример: кольцевой сдвиг

Каждый процесс отправляет свой элемент соседу справа и принимает от
соседа слева:

```cpp
int send_val = rank;
int recv_val = -1;

MPI_Request reqs[2];
MPI_Isend(&send_val, 1, MPI_INT, (rank + 1) % size, 0, MPI_COMM_WORLD, &reqs[0]);
MPI_Irecv(&recv_val, 1, MPI_INT, (rank - 1 + size) % size, 0, MPI_COMM_WORLD, &reqs[1]);
MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

// recv_val == (rank - 1 + size) % size
```

Если заменить на блокирующие `Send`/`Recv` в том же порядке — **дедлок**:
все процессы сначала `Send` и ждут, пока сосед вызовет `Recv`, а сосед
тоже ждёт. `Isend`/`Irecv` решают проблему, потому что не блокируются.

## `MPI_Waitall` vs `MPI_Wait`

- `MPI_Waitall(n, reqs, statuses)` — ждёт все, удобно для пар `Isend`+`Irecv`.
- `MPI_Wait(&req, &status)` — ждёт один, удобно в цикле по мере готовности.
- `MPI_Test` — если хотите делать полезную работу, пока операция не готова.

## Типичные ошибки

| Ошибка | Последствие |
|---|---|
| Менять буфер между `Isend` и `Wait` | Неопределённое поведение |
| Использовать `request` после `Wait` | Ошибка: `request` освобождён |
| Забыть `Wait`/`Test` для `Isend` | Утечка запроса, программа может завершиться до отправки |
| `MPI_Waitall` с `count`, не равным числу запросов | Чтение за границей массива |
| Смешивать `Isend` с блокирующим `Recv` без анализа порядка | Возможен дедлок |

## `MPI_Test` — неблокирующая проверка

```cpp
int flag = 0;
MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
if (flag) {
    // операция завершена, req освобождён
} else {
    // ещё летит, можно поработать
}
```

Полезно, когда между `Isend` и `Wait` есть вычисления: проверяете `Test`,
если готово — обрабатываете, если нет — считаете дальше.