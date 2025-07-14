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

#include "config.h"
#include "types.h"

/**
 * @brief Full persistent state saved in flash.
 *
 * Holds all known clients and their saved configurations.
 * Includes a CRC32 checksum for integrity verification.
 */
typedef struct {
    client_t clients[MAX_SERVER_CONNECTIONS];
    uint32_t crc;
} server_persistent_state_t;

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
 * @brief Prints the current GPIO state for a given client.
 *
 * @param client Pointer to the client_t structure to print.
 */
void server_print_running_client_state(const client_t *);

/**
 * @brief Prints a saved preset configuration for a given client.
 *
 * @param client Pointer to the client_t.
 * @param client_preset_index Preset index to print (0 to NUMBER_OF_POSSIBLE_PRESETS - 1).
 */
void server_print_client_preset_configuration(const client_t *, uint8_t);

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
        printf("%u. Device[%u]: UART connection, no access.\n",gpio_index + 1, gpio_index + 1);
    }else{
        printf("%u. Device[%u]: gpio_number:%u is_on:%u\n", gpio_index + 1, gpio_index + 1, client_state->devices[gpio_index].gpio_number, client_state->devices[gpio_index].is_on);
    }
}

/**
 * @brief Loads the server state from flash and validates it using CRC32.
 *
 * @param out_state Pointer to destination structure to store loaded state.
 * @return true if CRC is valid and data is intact, false otherwise.
 */
bool load_server_state(server_persistent_state_t *out_state);

/**
 * @brief Saves the persistent server state structure to flash memory.
 *
 * - Computes CRC
 * - Copies the data to a buffer
 * - Erases and programs the flash sector
 *
 * @param state_in Pointer to the server_persistent_state_t structure to save.
 */
void __not_in_flash_func(save_server_state)(const server_persistent_state_t *state_in);

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
void server_set_device_state_and_update_flash(uart_pin_pair_t , uart_inst_t*, uint8_t, bool, uint32_t);

/// Flash memory layout constants used for saving/loading persistent server state
#define SERVER_SECTOR_SIZE    4096
#define SERVER_PAGE_SIZE      256
#define SERVER_FLASH_OFFSET   (PICO_FLASH_SIZE_BYTES - SERVER_SECTOR_SIZE) ///< Offset from flash end
#define SERVER_FLASH_ADDR     (XIP_BASE + SERVER_FLASH_OFFSET)             ///< Runtime address of flash state

#endif