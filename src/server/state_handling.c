/**
 * @file state_handling.c
 * @brief Logic for loading, syncing, and managing client running states on the server.
 *
 * This file handles:
 * - Mapping between persistent flash state and active UART clients
 * - Loading each client's last known GPIO state and sending it via UART
 * - Verifying flash integrity using CRC and reinitializing if needed
 * - Managing dormant/active flags for each client based on GPIO activity
 *
 * Used during server startup or restart to ensure that connected clients resume
 * from their previous known state or are properly reinitialized if flash is invalid.
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "server.h"

uint32_t get_active_client_connection_index_from_flash_client_index(uint32_t flash_client_index, server_persistent_state_t state){
    for (uint8_t active_client_index = 0; active_client_index < active_server_connections_number; active_client_index++){
        if (active_uart_server_connections[active_client_index].pin_pair.tx == state.clients[flash_client_index].uart_connection.pin_pair.tx){
            return active_client_index;
        }
    }
    return INVALID_CLIENT_INDEX;
}

bool client_has_active_devices(client_t client){
    for (uint8_t device_index = 0; device_index < MAX_NUMBER_OF_GPIOS; device_index++){
        if (client.running_client_state.devices[device_index].is_on){
            return true;
        }
    }
    return false;
}

/**
 * @brief Updates the dormant status of all connected clients based on their active devices.
 *
 * For each active UART client, this function compares its TX pin with the
 * persistent state list and sets its `is_dormant` flag to true if all devices are OFF.
 *
 * @param server_persistent_state Pointer to the saved state containing all client info.
 */
static void set_dormant_flag_to_standby_clients(server_persistent_state_t *server_persistent_state){
    for (uint8_t active_client_index = 0; active_client_index < active_server_connections_number; active_client_index++){
        for (uint8_t persistent_state_client_index = 0; persistent_state_client_index < MAX_SERVER_CONNECTIONS; persistent_state_client_index++){
            if (active_uart_server_connections[active_client_index].pin_pair.tx == server_persistent_state->clients[persistent_state_client_index].uart_connection.pin_pair.tx){
                active_uart_server_connections[active_client_index].is_dormant = !client_has_active_devices(server_persistent_state->clients[persistent_state_client_index]);
                return;
            }
        }
    }
}

/**
 * @brief Loads the running state for an active client and sends it over UART.
 *
 * - Marks the client as active
 * - Sends the device states to the client
 *
 * @param server_uart_connection Connection info (pin pair + instance).
 * @param server_persistent_state Pointer to loaded flash state.
 */
static void server_load_client_state(server_uart_connection_t server_uart_connection, server_persistent_state_t *server_persistent_state) {
    for (uint8_t flash_client_index = 0; flash_client_index < MAX_SERVER_CONNECTIONS; flash_client_index++) {
        client_t *saved_client = &server_persistent_state->clients[flash_client_index];
        
        if (saved_client->uart_connection.pin_pair.tx == server_uart_connection.pin_pair.tx &&
            saved_client->uart_connection.pin_pair.rx == server_uart_connection.pin_pair.rx &&
            saved_client->uart_connection.uart_instance == server_uart_connection.uart_instance) {
                
            server_send_client_state(server_uart_connection.pin_pair, server_uart_connection.uart_instance, &saved_client->running_client_state);
            return;
        }
    }
}

void server_load_running_states_to_active_clients(void){
    server_persistent_state_t server_persistent_state = {0};
    bool valid_crc = load_server_state(&server_persistent_state);

    if (valid_crc) {
        printf("LOADING ATTEMPT SUCCESSFULL!\nLoading states.\n");
        for (uint8_t index = 0; index < active_server_connections_number; index++) {
            server_load_client_state(active_uart_server_connections[index], &server_persistent_state);
        }
    } else {
        printf("LOADING ATTEMPT FAILED!\nIncorrect CRC, this is the first run after build or might be a flash problem.\nInitializing Configuration...\n");
        server_configure_persistent_state(&server_persistent_state);
        printf("CONFIGURATION WAS SUCCESSFULL!\nStarting...\n");
    }

    set_dormant_flag_to_standby_clients(&server_persistent_state);
    send_dormant_to_standby_clients();
}
