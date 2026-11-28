add_executable(matvec_send main_send.cpp)
target_link_libraries(matvec_send PRIVATE common)

add_executable(matvec main_collective.cpp)
target_link_libraries(matvec PRIVATE common)