cmake_minimum_required(VERSION 3.16)
project(mpi_cpp_cheatsheet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(MPI REQUIRED COMPONENTS CXX)

add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(examples/01_hello)