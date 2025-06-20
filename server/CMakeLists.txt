cmake_minimum_required(VERSION 3.12)

project(server)

add_executable(server
    main.c
)

target_link_libraries(server pico_stdlib)

pico_add_extra_outputs(server)