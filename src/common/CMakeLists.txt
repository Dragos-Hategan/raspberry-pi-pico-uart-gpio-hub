# ---------------------------------------------------------------------------
# Common Module
#
# This CMake file defines a static library `common`, which provides:
# - General-purpose functions (LED control, UART I/O, etc.)
# - Type definitions and shared structures
# ---------------------------------------------------------------------------

add_library(common
    functions.c
    types.c
)

target_include_directories(common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
# Link against required Pico SDK libraries
target_link_libraries(common
    pico_stdlib          # Base I/O functions
    pico_stdio_usb
    hardware_clocks
    #hardware_powman #used only for RP2350
)

# Link Wi-Fi driver if supported
if(PICO_CYW43_SUPPORTED)
    target_link_libraries(common pico_cyw43_arch_none)
endif()

