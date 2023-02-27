cmake_minimum_required(VERSION 3.24)
project(modernCppWebServer)

#set(CMAKE_CXX_STANDARD 14 "-std=c++14 -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

add_executable(modernCppWebServer main.cpp pool/threadpool.h buffer/buffer.h buffer/buffer.cpp)
