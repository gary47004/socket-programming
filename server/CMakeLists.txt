project(server)
set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.13)

include_directories("./src")
file(GLOB _cpps $(./src/*/*.cpp))
message(STATUS "project cpps: ${_cpps}")

add_executable(server "cmd/server.cpp" ${_cpps})
target_link_libraries(server pthread)
