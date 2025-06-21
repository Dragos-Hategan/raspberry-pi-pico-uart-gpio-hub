cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(rpi-pico-uart-led-control C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
enable_language(CXX)

pico_sdk_init()

add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(common)