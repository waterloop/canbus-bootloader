cmake_minimum_required(VERSION 3.18)
project(bootloader_util C)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_STANDARD 11)

add_executable(bootloader_util
        canota.c
        main.c
        crc32.c)
