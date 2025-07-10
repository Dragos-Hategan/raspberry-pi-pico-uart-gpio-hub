cmake_minimum_required(VERSION 3.12)

project(server C)

add_executable(server
    main.c
    server_side_handshake.c
    state_handling.c
    input.c
    menu.c
)

pico_enable_stdio_usb(server 1)
pico_enable_stdio_uart(server 0)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link libraries: SDK + peripherals + common logic
target_link_libraries(server 
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_sync
    common           
)

# Optional Wi-Fi support if using CYW43 chip
if(PICO_CYW43_SUPPORTED)
    target_link_libraries(server pico_cyw43_arch_none)
endif()

# Generate UF2, bin, map, etc.
pico_add_extra_outputs(server)