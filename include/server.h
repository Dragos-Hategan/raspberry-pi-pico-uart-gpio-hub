/**
 * @file server.h
 * @brief Server-side functionality for UART-based Raspberry Pi Pico communication.
 *
 * Contains declarations for managing UART client detection, configuration, 
 * persistent state handling, and GPIO device state updates on connected clients.
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>

#include "hardware/uart.h"

#include "types.h"
#include "config.h"

/// Flash memory layout constants used for saving/loading persistent server state
#define SERVER_SECTOR_SIZE    4096
#define SERVER_PAGE_SIZE      256
#define SERVER_FLASH_OFFSET   (PICO_FLASH_SIZE_BYTES - SERVER_SECTOR_SIZE) ///< Offset from flash end
#define SERVER_FLASH_ADDR     (XIP_BASE + SERVER_FLASH_OFFSET)             ///< Runtime address of flash state

/**
 * @brief Active UART server connections detected at runtime.
 *
 * Filled after a successful scan in `server_find_connections()`.
 */
extern server_uart_connection_t active_uart_server_connections[MAX_SERVER_CONNECTIONS];

/**
 * @brief Number of active connections found and stored in `active_uart_server_connections`.
 */
extern uint8_t active_server_connections_number;

/**
 * @brief Finds the corresponding flash client index for a given active client.
 *
 * Compares the TX pin of the selected client with entries in the flash structure to
 * determine the matching flash client index.
 *
 * @param flash_client_index Output pointer to store the resolved flash index.
 * @param client_index Index of the selected client (1-based).
 * @param flash_state Pointer to the flash-stored server state.
 */
inline void find_corect_client_index_from_flash(uint32_t *flash_client_index, uint32_t client_index, const server_persistent_state_t *flash_state){
    for (uint8_t index = 0; index < MAX_SERVER_CONNECTIONS; index++){
        if (active_uart_server_connections[client_index - 1].pin_pair.tx == flash_state->clients[index].uart_connection.pin_pair.tx){
            *flash_client_index = index;
            break;
        }   
    }
}

/**
 * @brief Prints the GPIO state of a device at the given index in a client.
 *
 * This is an inline helper function. If the GPIO is used for UART, it is
 * marked as restricted.
 *
 * @param gpio_index Index of the device in the client_state array.
 * @param client_state Pointer to the client_state_t structure.
 */
inline void server_print_gpio_state(uint8_t gpio_index, const client_state_t *client_state){
    if (client_state->devices[gpio_index].gpio_number == UART_CONNECTION_FLAG_NUMBER){
        printf("%2u. UART connection, no access.\n", gpio_index + 1);
    }else{
        printf("%2u. GPIO_NO: %2u  Power: %s\n",
            gpio_index + 1,
            client_state->devices[gpio_index].gpio_number,
            client_state->devices[gpio_index].is_on ? "ON" : "OFF"
        );
    }
}

/**
 * @brief Prints the state of all devices from a given client state structure.
 *
 * - Iterates through all available GPIO devices.
 * - Prints each device's state using `server_print_gpio_state()`.
 * - Skips devices used for UART communication.
 *
 * @param client_state Pointer to the client_state_t structure to display.
 */
void server_print_state_devices(const client_state_t *client_state);

/**
 * @brief Prints the current GPIO state for a given client.
 *
 * @param client Pointer to the client_t structure to print.
 */
void server_print_running_client_state(const client_t * client);

/**
 * @brief Prints a saved preset configuration for a given client.
 *
 * @param client Pointer to the client_t.
 * @param client_preset_index Preset index to print (0 to NUMBER_OF_POSSIBLE_PRESETS - 1).
 */
void server_print_client_preset_configuration(const client_t * client, uint8_t client_preset_index);

/**
 * @brief Prints all preset configurations for a client.
 *
 * - Iterates through each preset slot of the client.
 * - Prints each configuration using `server_print_client_preset_configuration()`.
 * - Adds spacing between configurations for clarity.
 *
 * @param client Pointer to the client_t structure containing the preset configurations.
 */
void server_print_client_preset_configurations(const client_t * client);

/**
 * @brief Configures the devices for a given client and preset configuration index.
 *
 * This function enters an interactive loop where the user can set the ON/OFF state
 * for specific devices (GPIOs) of a selected client. The changes are stored in the 
 * specified preset configuration index within the server's persistent state.
 * 
 * The loop continues to prompt the user for device index and state until the input 
 * routine signals an exit (typically via a cancel command or invalid input).
 *
 * @param flash_client_index Index of the client in flash memory (persistent state).
 * @param flash_configuration_index Index of the preset configuration to modify.
 * @param input_client_data Pointer to a structure used for passing user input data.
 */
void set_configuration_devices(uint32_t flash_client_index, uint32_t flash_configuration_index, input_client_data_t *client_data);


/**
 * @brief Resets the currently active (running) configuration of a client.
 *
 * - Loads the full persistent server state from flash.
 * - Resets the `running_client_state` using `server_reset_configuration()`.
 * - Sends the updated state to the client over UART.
 * - Saves the modified state back to flash.
 *
 * @param flash_client_index Index of the client in the persistent flash structure.
 */
void reset_running_configuration(uint32_t flash_client_index);

/**
 * @brief Resets a specific preset configuration for a given client in flash.
 *
 * This function loads the current persistent server state from flash,
 * resets the specified preset configuration for the given client (excluding UART flags),
 * and then saves the updated state back to flash memory.
 *
 * @param[in] flash_client_index Index of the client in the flash-stored client list.
 * @param[in] flash_configuration_index Index of the preset configuration to reset (1-based).
 */
void reset_preset_configuration(uint32_t flash_client_index, uint32_t flash_configuration_index);

/**
 * @brief Resets all data associated with a client.
 *
 * - Resets the running state of the client and sends it over UART.
 * - Resets all preset configurations.
 * - Saves the updated state back to flash.
 *
 * @param flash_client_index Index of the client in the persistent flash structure.
 */
void reset_all_client_data(uint32_t flash_client_index);

/**
 * @brief Loads a preset configuration into a client's running state and applies it via UART.
 *
 * - Loads the full persistent state from flash.
 * - Copies the selected preset configuration into the running configuration.
 * - Sends the new configuration to the client via UART.
 * - Saves the updated state back to flash.
 *
 * @param flash_configuration_index Index of the preset configuration to load (0-based).
 * @param flash_client_index Index of the client in the flash-stored structure.
 */
void load_configuration_into_running_state(uint32_t flash_configuration_index, uint32_t flash_client_index);

/**
 * @brief Saves the current running configuration of a client into a preset slot.
 *
 * Loads the full persistent server state from flash, copies the active running state
 * of the specified client into the selected preset configuration slot, and saves
 * the updated structure back to flash.
 *
 * @param flash_configuration_index Index of the target preset configuration [0..(NUMBER_OF_POSSIBLE_PRESETS - 1)].
 * @param flash_client_index Index of the client within the persistent flash state.
 */
void save_running_configuration_into_preset_configuration(uint32_t flash_configuration_index, uint32_t flash_client_index);

/**
 * @brief Scans all possible UART pin pairs to detect connected clients.
 *
 * Performs UART handshakes and fills `active_uart_server_connections` with
 * valid connections.
 *
 * @return true if at least one client is found and validated, false otherwise.
 */
bool server_find_connections(void);

/**
 * @brief Loads saved GPIO states from flash and sends them to active clients.
 *
 * - Verifies CRC of saved state in flash.
 * - Sends current (running) state to each active client over UART.
 * - If CRC is invalid, calls `server_configure_persistent_state()` to reset flash.
 */
void server_load_running_states_to_active_clients(void);

/**
 * @brief Sets the state of a single device and updates flash accordingly.
 *
 * - Sends the updated GPIO state to the client via UART.
 * - Loads a copy of the persistent state from flash.
 * - Updates the state in RAM and saves the structure back to flash.
 *
 * @param pin_pair TX/RX pins for the UART connection to the client.
 * @param uart_instance UART peripheral used.
 * @param gpio_index GPIO number to modify on the client.
 * @param device_state true = ON, false = OFF.
 * @param flash_client_index Index of the client in flash storage.
 */
void server_set_device_state_and_update_flash(uart_pin_pair_t pin_pair, uart_inst_t* uart_instance, uint8_t gpio_index, bool device_state, uint32_t flash_client_index);

#endif