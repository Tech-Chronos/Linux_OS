cmake_minimum_required(VERSION 3.31)
project(thread)

set(CMAKE_CXX_STANDARD 20)

add_executable(thread main.cpp
        Thread.h
        BlockingQueue.h
        RingQueue.h
        ThreadPool.h)
