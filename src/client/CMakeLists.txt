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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
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

set(FLASH_OUT_DIR ${CMAKE_BINARY_DIR}/flash/client)

pico_add_extra_outputs(client)

add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${FLASH_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/client.uf2
        ${CMAKE_CURRENT_BINARY_DIR}/client.elf
        ${CMAKE_CURRENT_BINARY_DIR}/client.bin
        ${CMAKE_CURRENT_BINARY_DIR}/client.hex
        ${CMAKE_CURRENT_BINARY_DIR}/client.dis
        ${CMAKE_CURRENT_BINARY_DIR}/client.elf.map                        
        ${FLASH_OUT_DIR}
    COMMENT "Copying all client build outputs to flash/client/"
)