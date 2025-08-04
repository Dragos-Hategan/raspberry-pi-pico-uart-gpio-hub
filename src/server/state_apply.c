/**
 * @file state_apply.c
 * @brief Handles configuration management and device state updates for connected UART clients.
 *
 * This module provides functionality to:
 * - Send GPIO state updates to individual clients
 * - Save and load preset configurations for each client
 * - Reset running or preset client configurations
 * - Apply user input to modify preset configurations
 *
 * Used by the server to manage persistent client data and push changes
 * over UART with synchronization and power state awareness.
 *
 * @see server_persistent_state_t
 * @see client_state_t
 * @see input_client_data_t
 */

#include <string.h>

#include "server.h"
#include "input.h"

/**
 * @brief Sends the current state of a GPIO to a client device via UART.
 *
 * Wakes up the specified client using a TX pulse and then sends a message
 * representing the GPIO number and its current state (ON or OFF) in the format "[N,S]",
 * where `N` is the GPIO number and `S` is 1 (ON) or 0 (OFF).
 *
 * @param pin_pair            UART TX/RX pin pair used for the target client.
 * @param uart                Pointer to the UART instance used for transmission.
 * @param gpio_number         The GPIO number whose state is being sent.
 * @param is_on               The current state of the GPIO (`true` for ON, `false` for OFF).
 * @param state               Pointer to the global server persistent state (used to access client status).
 * @param flash_client_index  Index of the client in the persistent state table.
 *
 * @note This function uses `send_wakeup_if_dormant()` before sending the state,
 *       ensuring the client is awake before communication.
 *
 * @see send_wakeup_if_dormant()
 * @see send_uart_message_safe()
 */
static void server_send_device_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, uint8_t gpio_number, bool is_on, server_persistent_state_t *const state, uint32_t flash_client_index){
    send_wakeup_if_dormant(flash_client_index, state, pin_pair, uart);

    char msg[8];
    snprintf(msg, sizeof(msg), "[%d,%d]", gpio_number, is_on);
    send_uart_message_safe(uart, pin_pair, msg);
}

void server_set_device_state_and_update_flash(uart_pin_pair_t pin_pair, uart_inst_t* uart_instance, uint8_t gpio_index, bool device_state, uint32_t flash_client_index){
    server_persistent_state_t state_copy;
    memcpy(&state_copy, (const server_persistent_state_t *)SERVER_FLASH_ADDR, sizeof(state_copy));

    server_send_device_state(pin_pair, uart_instance, gpio_index, device_state, &state_copy, flash_client_index);
    state_copy.clients[flash_client_index].running_client_state.devices[gpio_index > 22 ? (gpio_index - 3) : (gpio_index)].is_on = device_state;
    save_server_state(&state_copy);
}

void save_running_configuration_into_preset_configuration(uint32_t flash_configuration_index, uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    memcpy(
        &state.clients[flash_client_index].preset_configs[flash_configuration_index],
        &state.clients[flash_client_index].running_client_state,
        sizeof(client_state_t));
    
    save_server_state(&state);

    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "\nConfiguration saved in Preset[%u].\n", flash_configuration_index + 1);
    printf_and_update_buffer(string);
}

void load_configuration_into_running_state(uint32_t flash_configuration_index, uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    memcpy(
        &state.clients[flash_client_index].running_client_state,
        &state.clients[flash_client_index].preset_configs[flash_configuration_index],
        sizeof(client_state_t));

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);
    save_server_state(&state);

    
    uint8_t active_client_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, state);
    if (!client_has_active_devices(state.clients[flash_client_index])){
        send_dormant_flag_to_client(active_client_index);
        active_uart_server_connections[active_client_index].is_dormant = true;
    }else{
        active_uart_server_connections[active_client_index].is_dormant = false;
    }

    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "\nConfiguration Preset[%u] Loaded!\n", flash_configuration_index + 1);
    printf_and_update_buffer(string);
}

void set_configuration_devices(uint32_t flash_client_index, uint32_t flash_configuration_index, input_client_data_t *input_client_data){
    server_persistent_state_t state;
    load_server_state(&state);

    while(true){
        uint32_t device_index;
        read_device_index(&device_index,
            flash_client_index,
            &state,
            &state.clients[flash_client_index].preset_configs[flash_configuration_index]
        );
        if (!device_index){
            return;
        }
    
        uint32_t device_state;
        read_device_state(&device_state);
        if (!device_state){
            return;
        }
        device_state %= 2;

        state.clients[flash_client_index].preset_configs[flash_configuration_index].devices[device_index - 1].is_on = device_state;

        save_server_state(&state);
    }
}

void reset_all_client_data(uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);
    server_reset_configuration(&state.clients[flash_client_index].running_client_state);

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);

    uint8_t active_client_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, state);
    send_dormant_flag_to_client(active_client_index);
    active_uart_server_connections[active_client_index].is_dormant = true;

    for (uint8_t configuration_index = 0; configuration_index < NUMBER_OF_POSSIBLE_PRESETS; configuration_index++){
        server_reset_configuration(&state.clients[flash_client_index].preset_configs[configuration_index]);
    }

    save_server_state(&state);
    printf_and_update_buffer("\nAll Client Data Reset.\n");
}

void reset_running_configuration(uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    server_reset_configuration(&state.clients[flash_client_index].running_client_state);

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);

    uint8_t active_client_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, state);
    send_dormant_flag_to_client(active_client_index);
    active_uart_server_connections[active_client_index].is_dormant = true;
    
    save_server_state(&state);

    printf_and_update_buffer("\nRunning Configuration Reset.\n");
}

void reset_preset_configuration(uint32_t flash_client_index, uint32_t flash_configuration_index){
    server_persistent_state_t state;
    load_server_state(&state);
    server_reset_configuration(&state.clients[flash_client_index].preset_configs[flash_configuration_index - 1]);

    save_server_state(&state);

    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "\nPreset Configuration [%u] Reset.\n", flash_configuration_index);
    printf_and_update_buffer(string);
}