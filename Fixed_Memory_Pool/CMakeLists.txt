cmake_minimum_required(VERSION 3.31)
project(Fixed_Memory_Pool)

set(CMAKE_CXX_STANDARD 20)

add_executable(Fixed_Memory_Pool main.cpp
        FixedSizePool.h)
