add_executable(matvec main.cpp)
target_link_libraries(matvec PRIVATE MPI::MPI_CXX)