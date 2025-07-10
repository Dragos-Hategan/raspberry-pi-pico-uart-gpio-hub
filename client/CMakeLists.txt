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

# Link against required Pico SDK libraries
target_link_libraries(client 
    pico_stdlib          # Base I/O functions
    hardware_uart        # UART peripheral access
    hardware_gpio        # GPIO peripheral access
    common
)

# Link Wi-Fi driver if supported
if(PICO_CYW43_SUPPORTED)
    target_link_libraries(client pico_cyw43_arch_none)
endif()

# Generate .uf2 and .bin outputs
pico_add_extra_outputs(client)