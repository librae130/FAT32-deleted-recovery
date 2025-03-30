cmake_minimum_required(VERSION 3.10)

project(FAT32R)
project(FAT32R VERSION 1.0)

add_executable(FAT32R main.cpp Fat32.cpp Fat32Recoverer.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)


