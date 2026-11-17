add_executable(hello main.cpp)
target_link_libraries(hello PRIVATE MPI::MPI_CXX)