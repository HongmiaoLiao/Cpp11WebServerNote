cmake_minimum_required(VERSION 3.24)
project(modernCppWebServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(modernCppWebServer main.cpp pool/threadpool.h)
