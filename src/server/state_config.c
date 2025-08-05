/**
 * @file state_config.c
 * @brief Configuration logic for initializing client states and presets on the UART server.
 *
 * This file handles:
 * - Setting up initial GPIO states (both running and presets) for each client
 * - Reserving UART communication pins to avoid GPIO conflicts
 * - Initializing client UART connection parameters
 * - Providing reset and configuration routines for persistent state management
 *
 * Functions in this file are mainly called during boot or full system reset,
 * and ensure all clients start with a known clean state.
 * 
 */

#include "server.h"

/**
 * @brief Marks UART pins as reserved in the preset configurations.
 *
 * @param client_list_index Index of the client.
 * @param server_persistent_state Persistent state pointer.
 * @param config_index Preset configuration index to modify.
 */
static void configure_preset_configs_uart_connection_pins(uint8_t client_list_index, server_persistent_state_t *server_persistent_state, uint8_t config_index){
    for (uint8_t index = 0; index < active_server_connections_number; index++) {
        client_t *client = &server_persistent_state->clients[client_list_index];
        if (active_uart_server_connections[index].pin_pair.tx == client->uart_connection.pin_pair.tx){
            client->preset_configs[config_index].devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.tx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
            client->preset_configs[config_index].devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.rx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
            return;
        }
    }
}

/**
 * @brief Initializes all preset configurations for a client.
 *
 * - Fills in GPIO numbers and disables all.
 * - Marks UART pins as reserved.
 *
 * @param client_list_index Client index.
 * @param server_persistent_state Persistent state struct.
 */
static void configure_preset_configs(uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t config_index = 0; config_index < NUMBER_OF_POSSIBLE_PRESETS; config_index++){
        for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
            server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[gpio_index].gpio_number = gpio_index + ((gpio_index / 23) * 3);
            server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[gpio_index].is_on = false;
        }

        configure_preset_configs_uart_connection_pins(client_list_index, server_persistent_state, config_index);
    }
}

/**
 * @brief Marks UART pins as reserved in the running config of a client.
 *
 * - Used to prevent GPIO conflicts for UART communication pins.
 *
 * @param client_list_index Index of the client in flash.
 * @param server_persistent_state Pointer to full persistent state.
 */
static void configure_running_state_uart_connection_pins(uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t index = 0; index < active_server_connections_number; index++) {
        client_t *client = &server_persistent_state->clients[client_list_index];
        if (active_uart_server_connections[index].pin_pair.tx == client->uart_connection.pin_pair.tx){
            client->running_client_state.devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.tx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
            client->running_client_state.devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.rx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
            return;
        }
    }
}

/**
 * @brief Initializes the current (live) GPIO states for a client.
 *
 * - Assigns GPIO numbers and sets all to off.
 * - Flags UART pins to avoid conflict.
 *
 * @param client_list_index Index of the client.
 * @param server_persistent_state Persistent state structure.
 */
static void configure_running_state(uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t index = 0; index < MAX_NUMBER_OF_GPIOS; index++){
        server_persistent_state->clients[client_list_index].running_client_state.devices[index].gpio_number = index + ((index / 23) * 3);
        server_persistent_state->clients[client_list_index].running_client_state.devices[index].is_on = false;
    }

    configure_running_state_uart_connection_pins(client_list_index, server_persistent_state);
}

/**
 * @brief Initializes a client entry in the persistent state.
 *
 * - Saves UART pin pair and instance.
 * - Sets is_dormant to true.
 * - Calls config functions.
 *
 * @param uart_pin_pair TX/RX pin pair for the client.
 * @param client_list_index Index in client array.
 * @param server_persistent_state State structure to populate.
 * @param uart_inst UART instance.
 */
static void configure_client(uart_pin_pair_t uart_pin_pair, uint8_t client_list_index, server_persistent_state_t *server_persistent_state, uart_inst_t* uart_inst){
    server_persistent_state->clients[client_list_index].uart_connection.pin_pair = uart_pin_pair;
    server_persistent_state->clients[client_list_index].uart_connection.uart_instance = uart_inst;

    configure_running_state(client_list_index, server_persistent_state);
    configure_preset_configs(client_list_index, server_persistent_state);
}

void server_configure_persistent_state(server_persistent_state_t *server_persistent_state) {
    uint8_t client_list_index = 0;
    for (uint8_t i = 0; i < PIN_PAIRS_UART0_LEN; i++) {
        configure_client(pin_pairs_uart0[i], client_list_index, server_persistent_state, uart0);
        client_list_index++;
    }
    for (uint8_t i = 0; i < PIN_PAIRS_UART1_LEN; i++) {
        configure_client(pin_pairs_uart1[i], client_list_index, server_persistent_state, uart1);
        client_list_index++;
    }
    save_server_state(server_persistent_state);
}

void server_reset_configuration(client_state_t *client_state){
    for (uint8_t index = 0; index < MAX_NUMBER_OF_GPIOS; index++){
        if (client_state->devices[index].gpio_number != UART_CONNECTION_FLAG_NUMBER){
            client_state->devices[index].is_on = false;
        }
    }
}