cmake_minimum_required(VERSION 3.12)

project(server C)

add_executable(server
    main.c
)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(server 
    pico_stdlib
    hardware_uart
    hardware_gpio
    common
)

if(PICO_CYW43_SUPPORTED)
    target_link_libraries(server pico_cyw43_arch_none)
endif()

pico_add_extra_outputs(server)