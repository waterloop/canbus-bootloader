cmake_minimum_required(VERSION 3.18)
project(canota C)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_STANDARD 11)

add_executable(canota
        canota.c
        main.c
        crc32.c)
