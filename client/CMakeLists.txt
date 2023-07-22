project(client)
set(CMAKE_CXX_STANDARD 17)
add_executable(client "client.cpp")
target_link_libraries(client pthread)
