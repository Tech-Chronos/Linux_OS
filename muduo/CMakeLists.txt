cmake_minimum_required(VERSION 3.10)
project(EchoDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 调试时常用
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra")

# 当前目录就是头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 服务端
add_executable(server
    server.cc
)

target_link_libraries(server pthread)

# 客户端
add_executable(client
    client.cc
)

target_link_libraries(client pthread)