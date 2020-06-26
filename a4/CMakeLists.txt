cmake_minimum_required(VERSION 3.15)
project(a4 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(a4
        controller.c
        controller.h
        gateway.c
        humidity.c
        message.c
        message.h
        socket.c
        socket.h
        temperature.c)
