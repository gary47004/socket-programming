project(server)
set(CMAKE_CXX_STANDARD 17)

file(GLOB _cpps $(./src/*.cpp))
message(STATUS "project cpps: ${_cpps}")

add_executable(server "server.cpp" ${_cpps})
target_link_libraries(server pthread)
