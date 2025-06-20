cmake_minimum_required(VERSION 3.12)

include(../pico_sdk_import.cmake)

project(server)

pico_sdk_init()

add_executable(server
    main.c
)

target_link_libraries(server pico_stdlib)

pico_add_extra_outputs(server)