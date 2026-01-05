cmake_minimum_required(VERSION 3.15...4.1)


option(WITH_EXAMPLES "Build examples" OFF)
option(WITH_TESTS "Build test" OFF)

project(
  cson
  LANGUAGES CXX)

add_library(cson src/cson.cpp include/cson.h)

# treat warnings as error
set_property(TARGET cson PROPERTY COMPILE_WARNING_AS_ERROR ON)

# enable c++ 11
target_compile_features(cson PRIVATE cxx_std_11)

target_include_directories(cson PUBLIC include)

if(WITH_EXAMPLES)
    add_subdirectory(examples)
endif()

if(WITH_TESTS)
    add_subdirectory(test)
endif()
