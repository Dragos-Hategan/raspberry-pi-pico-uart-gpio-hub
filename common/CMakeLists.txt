add_library(common
    functions.c
    config.c
)

target_include_directories(common PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(common pico_stdlib)

if(PICO_CYW43_SUPPORTED)
    target_link_libraries(common pico_cyw43_arch_none)
endif()

