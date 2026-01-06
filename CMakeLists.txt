cmake_minimum_required(VERSION 3.23)


option(WITH_EXAMPLES "Build examples" OFF)
option(WITH_TESTS "Build test" OFF)

project(
  cson
  LANGUAGES CXX)


if (MSVC)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")
endif()

add_library(cson)

target_sources(cson
    PUBLIC
        src/cson.cpp
    PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES include/cson.h
)

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

install(
    TARGETS cson
    FILE_SET HEADERS
)
