cmake_minimum_required(VERSION 3.15...4.1)

add_executable(parse1 parse1.cpp)

target_link_libraries(parse1 PUBLIC cson)

