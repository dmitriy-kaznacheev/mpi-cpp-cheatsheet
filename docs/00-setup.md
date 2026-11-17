# 00 — Установка и окружение

## Что нужно

- CMake ≥ 3.16
- Компилятор C++17 (g++ ≥ 9, clang++ ≥ 10)
- MPI-реализация: OpenMPI или MPICH

## Установка MPI

**Debian/Ubuntu (OpenMPI):**

    sudo apt install openmpi-bin libopenmpi-dev

**Debian/Ubuntu (MPICH):**

    sudo apt install mpich libmpich-dev

**Fedora:**

    sudo dnf install openmpi openmpi-devel

**macOS (Homebrew):**

    brew install open-mpi

**Проверка:**

    mpicxx --version
    mpirun --version

## Сборка примеров

Из корня репозитория:

    cmake -S . -B build
    cmake --build build -j

Бинарники складываются в `build/bin/`.

Собрать только один пример:

    cmake --build build --target hello

## Запуск

    mpirun -np 4 ./build/bin/hello

- `-np N` — число процессов.
- Если ядер меньше, чем `N`, добавьте `--oversubscribe`:
  `mpirun --oversubscribe -np 4 ./build/bin/hello`
- Под root (в контейнере) OpenMPI требует:
  `mpirun --allow-run-as-root -np 4 ./build/bin/hello`

## Полезные переменные окружения

| Переменная | Назначение |
|---|---|
| `OMPI_MCA_btl_vader_single_copy_mechanism=none` | Обход проблем с shared memory в контейнерах |
| `OMPI_ALLOW_RUN_AS_ROOT=1` + `OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1` | Запуск под root без флага |
| `MPIEXEC_NUMPROC_FLAG` | Флаг числа процессов (используется CMake, обычно `-n`) |

## IDE

Для clangd / VS Code сгенерируйте `compile_commands.json`:

    cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ln -s build/compile_commands.json compile_commands.json

## Частые проблемы

| Симптом | Причина / решение |
|---|---|
| `find_package(MPI)` не находит MPI | Не установлен `*-dev` пакет, либо CMake ищет не ту реализацию. Задайте `MPI_CXX_COMPILER=mpicxx` |
| `mpirun` не запускается под root | Добавьте `--allow-run-as-root` |
| `not enough slots available` | Ядер меньше, чем `-np`. Добавьте `--oversubscribe` |
| Программа виснет на `MPI_Recv` | Непарный `Send`/`Recv`, неверный тег или коммуникатор (см. `08-pitfalls.md`) |