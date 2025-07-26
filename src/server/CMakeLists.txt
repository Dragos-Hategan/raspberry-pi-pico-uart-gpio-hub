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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Link libraries: SDK + peripherals + common logic
target_link_libraries(server 
    pico_stdlib
    pico_time
    hardware_watchdog
    hardware_uart
    hardware_gpio
    common           
)

# Optional Wi-Fi support if using CYW43 chip
if(PICO_CYW43_SUPPORTED)
    target_link_libraries(server pico_cyw43_arch_none)
endif()

set(FLASH_OUT_DIR ${CMAKE_BINARY_DIR}/flash/server)

pico_add_extra_outputs(server)

add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${FLASH_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/server.uf2
        ${CMAKE_CURRENT_BINARY_DIR}/server.elf
        ${CMAKE_CURRENT_BINARY_DIR}/server.bin
        ${CMAKE_CURRENT_BINARY_DIR}/server.hex
        ${CMAKE_CURRENT_BINARY_DIR}/server.elf.map
        ${CMAKE_CURRENT_BINARY_DIR}/server.dis
        ${FLASH_OUT_DIR}
    COMMENT "Copying all server build outputs to flash/server/"
)