/**
 * @file state_print.c
 * @brief UI printing utilities for displaying client GPIO states on the server interface.
 *
 * This file provides:
 * - Functions to print the current (running) GPIO state of a client
 * - Functions to print each preset configuration associated with a client
 * - UART-protected GPIOs are identified and marked as restricted
 *
 * The output is formatted and routed through `printf_and_update_buffer()`,
 * making it suitable for CLI menus or serial interfaces.
 *
 * Typical use cases include state visualization during debugging, runtime inspection,
 * and user interaction through the USB CLI.
 *
 */

#include "server.h"
#include "menu.h"

/**
 * @brief Prints the GPIO state of a device at the given index in a client.
 *
 * This is an inline helper function. If the GPIO is used for UART, it is
 * marked as restricted.
 *
 * @param gpio_index Index of the device in the client_state array.
 * @param client_state Pointer to the client_state_t structure.
 */
static void server_print_gpio_state(uint8_t gpio_index, const client_state_t *client_state){
    if (client_state->devices[gpio_index].gpio_number == UART_CONNECTION_FLAG_NUMBER){
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "%2u. UART connection, no access.\n", gpio_index + 1);
        printf_and_update_buffer(string);
    }else{
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "%2u. GPIO_NO: %2u  Power: %s\n",
            gpio_index + 1,
            client_state->devices[gpio_index].gpio_number,
            client_state->devices[gpio_index].is_on ? "ON" : "OFF");
        printf_and_update_buffer(string);
    }
}

void server_print_state_devices(const client_state_t *client_state){
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, client_state);
    }
}

void server_print_running_client_state(const client_t *client){
    printf_and_update_buffer("Running Client State Devices:\n");
    server_print_state_devices(&client->running_client_state);
}

void server_print_client_preset_configuration(const client_t *client, uint8_t client_preset_index){
    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "Preset Config[%u] Devices:\n", client_preset_index + 1);
    printf_and_update_buffer(string);
    server_print_state_devices(&client->preset_configs[client_preset_index]);
}

void server_print_client_preset_configurations(const client_t *client){
    for (uint32_t preset_config_index = 0; preset_config_index < NUMBER_OF_POSSIBLE_PRESETS; preset_config_index++){
        server_print_client_preset_configuration(client, preset_config_index);
        printf_and_update_buffer("\n");
    }
}