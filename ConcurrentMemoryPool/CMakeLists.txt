cmake_minimum_required(VERSION 3.31)
project(ConcurrentMemoryPool)

set(CMAKE_CXX_STANDARD 20)

add_executable(ConcurrentMemoryPool unit_test.cpp
        FixedMemoryPool.h
        ThreadCache.h
        ThreadCache.cc
        Common.h
        ConCurrent.h
        CentralCache.cpp
        CentralCache.h
        PageCache.h
        PageCache.cpp
)
