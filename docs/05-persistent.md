# 05 — Persistent-запросы

Persistent-запросы — это заранее созданные «шаблоны» операций
`Send`/`Recv`. Инициализация (`Send_init`/`Recv_init`) происходит один раз,
а запуск (`Start`/`Startall`) — сколько угодно раз. Полезно, когда один
и тот же обмен повторяется в цикле: итерационные методы, обмен границами
в PDE, кольцевые сдвиги.

## Зачем

1. **Меньше накладных расходов.** `Send_init`/`Recv_init` делают всю
   «бумажную работу» (проверка аргументов, подготовка внутренних структур)
   один раз. `Start` дешевле, чем `MPI_Send`/`MPI_Recv`.
2. **Буфер фиксирован.** Persistent-запрос «привязан» к буферу, который
   был передан в `*_init`. Это удобно для колец и границ: буфер один,
   меняется только содержимое.
3. **Меньше ошибок.** Нельзя случайно перепутать `count`/`datatype` между
   итерациями — они зафиксированы при инициализации.

## Основные функции

```cpp
int MPI_Send_init(const void* buf, int count, MPI_Datatype dt,
                  int dest, int tag, MPI_Comm comm, MPI_Request* request);

int MPI_Recv_init(void* buf, int count, MPI_Datatype dt,
                  int source, int tag, MPI_Comm comm, MPI_Request* request);

int MPI_Start(MPI_Request* request);
int MPI_Startall(int count, MPI_Request* requests);
```

- `*_init` **не отправляет и не принимает** — только создаёт запрос.
- `Start`/`Startall` запускают операцию. После запуска — как `Isend`/`Irecv`:
  буфер принадлежит MPI до `Wait`.
- `MPI_Request_free` освобождает persistent-запрос (после `Wait` или
  если он больше не нужен). Без `Request_free` — утечка.

## Жизненный цикл

```cpp
MPI_Request req;
MPI_Send_init(buf, n, MPI_DOUBLE, dest, tag, comm, &req);

for (int iter = 0; iter < n_iters; ++iter) {
    // заполнить buf новыми данными
    MPI_Start(&req);
    // ... можно делать вычисления, пока идёт отправка ...
    MPI_Wait(&req, MPI_STATUS_IGNORE);
}

MPI_Request_free(&req);
```

## Пример: кольцевой сдвиг в цикле

```cpp
MPI_Request reqs[2];
int send_val, recv_val;

MPI_Send_init(&send_val, 1, MPI_INT, (rank + 1) % size, 0, comm, &reqs[0]);
MPI_Recv_init(&recv_val, 1, MPI_INT, (rank - 1 + size) % size, 0, comm, &reqs[1]);

for (int iter = 0; iter < n_iters; ++iter) {
    send_val = rank * 1000 + iter;   // новое значение каждый раз
    MPI_Startall(2, reqs);
    MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);
    // recv_val содержит значение от соседа слева
}

MPI_Request_free(&reqs[0]);
MPI_Request_free(&reqs[1]);
```

Без persistent пришлось бы каждый раз вызывать `MPI_Isend`/`MPI_Irecv`,
что дороже на больших `n_iters`.

## `MPI_Startall` vs `MPI_Start`

- `Startall(n, reqs)` — запустить все запросы из массива (типично для пары
  `Send`+`Recv`).
- `Start(&req)` — запустить один (если их много и они независимы).

## Типичные ошибки

| Ошибка | Последствие |
|---|---|
| `Start` без `Wait` перед следующим `Start` | Неопределённое поведение: запрос ещё активен |
| Менять буфер между `Start` и `Wait` | Неопределённое поведение (MPI использует буфер) |
| Забыть `MPI_Request_free` | Утечка запроса (не ошибка, но ресурсы не освобождены) |
| `MPI_Request_free` до `Wait` | Операция может не завершиться, буфер освобождён — UB |
| Использовать `Send_init` для одного обмена | Накладные расходы больше, чем у `MPI_Send` — не имеет смысла |

## Когда **не** использовать

- Один обмен — persistent дороже, чем обычный `Send`/`Recv`.
- Число итераций мало (1–2) — выгода не покроет инициализацию.
- Параметры обмена меняются между итерациями (другой `dest`, `count`,
  `tag`) — persistent их фиксирует, придётся пересоздавать.

## Связь с `Isend`/`Irecv`

| Обычные | Persistent |
|---|---|
| `MPI_Isend`/`MPI_Irecv` каждый раз | `MPI_Send_init`/`MPI_Recv_init` один раз |
| Параметры можно менять | Параметры фиксированы |
| Накладные расходы на каждый вызов | Только на `Start`/`Startall` |
| `MPI_Wait` освобождает запрос | `MPI_Wait` не освобождает, нужен `Request_free` |