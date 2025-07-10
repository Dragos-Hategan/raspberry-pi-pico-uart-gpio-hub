cmake_minimum_required(VERSION 3.12)

project(client C)

add_executable(client
    main.c
    client_side_handshake.c
    apply_commands.c
)

pico_enable_stdio_usb(client 1)
pico_enable_stdio_uart(client 0)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
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