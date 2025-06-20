cmake_minimum_required(VERSION 3.12)

include(../pico_sdk_import.cmake)

project(client)

pico_sdk_init()

add_executable(client
    main.c
)

target_link_libraries(client pico_stdlib)

pico_add_extra_outputs(client)