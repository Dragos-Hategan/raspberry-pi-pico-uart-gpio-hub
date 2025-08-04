cmake_minimum_required(VERSION 3.12)

project(server C)

add_executable(server
    client_communication.c
    input.c
    main.c
    menu.c
    server_side_handshake.c
    state_apply.c
    state_config.c
    state_flash.c
    state_handling.c
    state_print.c
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
    pico_multicore
    hardware_watchdog
    hardware_uart
    hardware_gpio
    common
)

if(DEFINED PERIODIC_ONBOARD_LED_BLINK_SERVER)
    target_compile_definitions(server PRIVATE PERIODIC_ONBOARD_LED_BLINK_SERVER=${PERIODIC_ONBOARD_LED_BLINK_SERVER})
endif()

if(DEFINED RESTART_SYSTEM_AT_USB_RECONNECTION)
    target_compile_definitions(server PRIVATE RESTART_SYSTEM_AT_USB_RECONNECTION=${RESTART_SYSTEM_AT_USB_RECONNECTION})
endif()

if(DEFINED PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS)
    target_compile_definitions(server PRIVATE PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS=${PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS})
endif()

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