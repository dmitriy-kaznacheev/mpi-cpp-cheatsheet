add_executable(matvec_send main_send.cpp)
target_link_libraries(matvec_send PRIVATE MPI::MPI_CXX)

add_executable(matvec main_collective.cpp)
target_link_libraries(matvec PRIVATE MPI::MPI_CXX)