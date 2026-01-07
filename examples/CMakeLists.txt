cmake_minimum_required(VERSION 3.15...4.1)

add_executable(parse1 parse1.cpp)
target_link_libraries(parse1 PUBLIC cson)


add_executable(csonbin cson.cpp)
set_target_properties(csonbin PROPERTIES OUTPUT_NAME cson)
target_link_libraries(csonbin PUBLIC cson)
