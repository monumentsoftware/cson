include(CTest)

cmake_minimum_required(VERSION 3.15...4.1)


add_executable(test test.cpp)
target_link_libraries(test PRIVATE cson)

enable_testing()
add_test(NAME csontest COMMAND test)
