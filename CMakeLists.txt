cmake_minimum_required(VERSION 3.15...4.1)

project(
  cson
  LANGUAGES CXX)

add_library(cson src/cson.cpp include/cson.h)

# enable c++ 11
target_compile_features(cson PRIVATE cxx_std_11)

target_include_directories(cson PUBLIC include)

