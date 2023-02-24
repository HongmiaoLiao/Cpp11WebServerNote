cmake_minimum_required(VERSION 3.21)
project(TinyWebServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/lib64/mysql/ -lmysqlclient -lpthread")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -mysqlclient")

include_directories(.)
include_directories(CGImysql)
include_directories(http)
include_directories(lock)
include_directories(log)
include_directories(threadpool)
include_directories(timer)

add_executable(TinyWebServer
        CGImysql/sql_connection_pool.cpp
        CGImysql/sql_connection_pool.h
        http/http_conn.cpp
        http/http_conn.h
        lock/locker.h
        log/block_queue.h
        log/log.cpp
        log/log.h
        threadpool/threadpool.h
        timer/lst_timer.cpp
        timer/lst_timer.h
        config.cpp
        config.h
        main.cpp
        webserver.cpp
        webserver.h)
