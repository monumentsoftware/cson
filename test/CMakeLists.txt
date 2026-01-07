include(CTest)

cmake_minimum_required(VERSION 3.15...4.1)

add_subdirectory(googletest EXCLUDE_FROM_ALL)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE cson GTest::gtest_main)

enable_testing()

include(GoogleTest)
gtest_discover_tests(test)
