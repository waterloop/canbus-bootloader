cmake_minimum_required(VERSION 3.00)

set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(bootloader LANGUAGES C ASM)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

file(GLOB board_files "boards/*")

foreach(board_file IN ITEMS ${board_files})
    get_filename_component(board ${board_file} NAME)

    file(GLOB_RECURSE sources "src/*.c" "src/*.s" "boards/${board}/startup.s" "boards/${board}/system.c")

    set(linker_script "${CMAKE_SOURCE_DIR}/boards/${board}/bootloader.ld")

    include("boards/${board}/vars.cmake")

    add_executable(${board}.elf)
    target_sources(${board}.elf PRIVATE ${sources})
    target_include_directories(${board}.elf PRIVATE "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/include/system" "${CMAKE_SOURCE_DIR}/boards/${board}" "${CMAKE_SOURCE_DIR}/boards/${board}/system")
    target_compile_options(${board}.elf PRIVATE -mcpu=${CPU} -mthumb -Wall -std=c11 -MMD -Og -fno-builtin -ffunction-sections -fdata-sections -g)
    target_link_options(${board}.elf PRIVATE -mcpu=${CPU} -mthumb -nolibc -Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/${board}.map -Wl,--print-memory-usage -T${linker_script})
    set_target_properties(${board}.elf PROPERTIES LINK_DEPENDS ${linker_script})

    add_custom_target(${board}.bin ALL DEPENDS ${board}.elf)
    add_custom_command(TARGET ${board}.bin COMMAND ${CMAKE_OBJCOPY} ARGS -O binary ${board}.elf ${board}.bin)
endforeach()