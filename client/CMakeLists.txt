cmake_minimum_required(VERSION 3.12)

project(client C)

add_executable(client
    main.c
)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

target_link_libraries(client 
    pico_stdlib
    hardware_uart
    hardware_gpio
    common
)

if(PICO_CYW43_SUPPORTED)
    target_link_libraries(client pico_cyw43_arch_none)
endif()

pico_add_extra_outputs(client)