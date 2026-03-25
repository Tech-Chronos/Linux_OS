cmake_minimum_required(VERSION 3.31)
project(Pool)

set(CMAKE_CXX_STANDARD 20)

add_executable(Pool unitest.cpp
        ThreadCache.cpp
        ThreadCache.h
        Common.h
        CentralCache.cpp
        CentralCache.h
        PageCache.cpp
        PageCache.h
        Concurrent.h
        BenchMark.cpp
        ObjectPool.h
        RadixTree.h)
