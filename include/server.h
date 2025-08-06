/**
 * @file server.h
 * @brief Central interface for server-side UART client management and GPIO state handling.
 *
 * This header declares all server-side functions and variables used for:
 * - Discovering UART-connected clients
 * - Managing persistent state (running and preset configurations)
 * - Communicating with clients via UART
 * - Flash memory operations (load/save with CRC)
 * - Resetting, loading, and printing client GPIO configurations
 * - Handling dormant/active state transitions
 *
 * It serves as the main coordination point between UART communication,
 * persistent memory, and user interface logic via the USB CLI.
 *
 * All modules (config, flash, menu, CLI, and communication) include and rely on this header.
 *
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>

#include "hardware/uart.h"
#include "pico/multicore.h"

#include "menu.h"
#include "types.h"
#include "config.h"

/// Flash memory layout constants used for saving/loading persistent server state
#define SERVER_SECTOR_SIZE    4096
#define SERVER_PAGE_SIZE      256
#define SERVER_FLASH_OFFSET   (PICO_FLASH_SIZE_BYTES - SERVER_SECTOR_SIZE) ///< Offset from flash end
#define SERVER_FLASH_ADDR     (XIP_BASE + SERVER_FLASH_OFFSET)             ///< Runtime address of flash state
#define INVALID_CLIENT_INDEX -1

#ifndef UART_SPINLOCK_ID
#define UART_SPINLOCK_ID 0
#endif

extern spin_lock_t *uart_lock;

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
 * @brief Sends a UART message safely using spinlock protection.
 *
 * This function first wakes up the client by sending a pulse on its RX pin.
 * Then it initializes the UART peripheral with the specified TX/RX pins and baudrate,
 * sends the message, waits for the transmission to complete, and resets the GPIO pins.
 * All UART operations are protected by a spinlock to ensure thread/core safety.
 *
 * @param uart Pointer to the UART instance to use (e.g., uart0 or uart1).
 * @param pins Struct containing the TX and RX GPIO pin numbers.
 * @param msg Null-terminated string to be sent via UART.
 */
void send_uart_message_safe(uart_inst_t* uart, uart_pin_pair_t pins, const char* msg);

/**
 * @brief Wakes up a dormant client if needed, based on its persistent state.
 *
 * Checks whether the client at the specified index is dormant. If so, it
 * sends a wake-up pulse via UART to ensure the client is active before communication.
 *
 * @param flash_client_index Index of the client in the persistent state table.
 * @param state              Pointer to the global persistent state structure.
 * @param pin_pair           UART TX/RX pins used to wake up the client.
 * @param uart               Pointer to the UART instance used for transmission.
 *
 * @see wake_up_client()
 */
void send_wakeup_if_dormant(uint32_t flash_client_index, server_persistent_state_t *const state, uart_pin_pair_t pin_pair, uart_inst_t *const uart);

/**
 * @brief Sends a dormant flag message to a specific client over UART.
 *
 * Constructs a message with the dormant flag number and sends it
 * via the UART instance and pin pair assigned to the specified client.
 *
 * @param client_index Index of the client in the active server connections.
 */
void send_dormant_flag_to_client(uint8_t client_index);

/**
 * @brief Sends a reset trigger message to all clients.
 *
 * This causes each connected client to perform a full reset of its internal state.
 */
void signal_reset_for_all_clients();

/**
 * @brief Sends a fast onboard LED blink signal to all clients.
 *
 * Triggers a visual blink (e.g., heartbeat) on each client device.
 */
void send_fast_blink_onboard_led_to_clients();

/**
 * @brief Sends a dormant message to all clients marked as dormant.
 *
 * Iterates through all active UART connections and sends a predefined
 * dormant flag message to each client with the `is_dormant` flag set.
 */
void send_dormant_to_standby_clients(void);

/**
 * @brief Sends the entire current client state over UART.
 *
 * @param pin_pair UART TX/RX pin pair to use.
 * @param uart UART instance.
 * @param state Pointer to the client_state_t to send.
 */
void server_send_client_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, const client_state_t* state);

/**
 * @brief Core1 wakeup handler triggered by inter-core messages.
 *
 * Handles two commands:
 * - `DUMP_BUFFER_WAKEUP_MESSAGE`: Reprints stored output to CLI.
 * - `BLINK_LED_WAKEUP_MESSAGE`: Triggers fast onboard LED blink and mirrors to clients.
 */
void periodic_wakeup(void);

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
 * @brief Loads a saved preset configuration into a client's running state.
 *
 * Copies the selected preset configuration into the client's current state,
 * sends the new state to the client via UART, and updates the persistent flash.
 * If the loaded configuration results in all devices being OFF, the client is
 * marked as dormant.
 *
 * A confirmation message is printed to the USB CLI.
 *
 * @param flash_configuration_index Index of the preset configuration to load.
 * @param flash_client_index Index of the target client in the persistent flash state.
 */
void load_configuration_into_running_state(uint32_t flash_configuration_index, uint32_t flash_client_index);

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
 * @param client_data Pointer to a structure used for passing user input data.
 */
void set_configuration_devices(uint32_t flash_client_index, uint32_t flash_configuration_index, input_client_data_t *client_data);

/**
 * @brief Resets all configuration data for a specified client.
 *
 * This includes:
 * - Resetting the client's current (running) configuration
 * - Sending the reset state to the client via UART
 * - Marking the client as dormant
 * - Resetting all preset configurations associated with the client
 * - Saving the updated state to flash
 *
 * @param flash_client_index Index of the client in the persistent flash state array.
 */
void reset_all_client_data(uint32_t flash_client_index);

/**
 * @brief Resets the running configuration of a specified client to its default state.
 *
 * Loads the server's persistent state, resets the client's current configuration,
 * sends the updated state to the client, marks the client as dormant, and saves
 * the updated state back to flash.
 *
 * @param flash_client_index Index of the client in the persistent flash state array.
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
 * @brief Fills the entire persistent state structure and saves it to flash.
 *
 * - Called when flash is empty or invalid.
 * - Configures all known clients.
 *
 * @param server_persistent_state Pointer to state struct to fill and write.
 */
void server_configure_persistent_state(server_persistent_state_t *server_persistent_state);

/**
 * @brief Resets the runtime GPIO configuration for a given client.
 *
 * This function iterates through all devices associated with the client
 * and sets their `is_on` flag to false, effectively turning them off.
 * Devices marked with the special UART connection flag are excluded from this reset.
 *
 * @param[in,out] client_state Pointer to the client's current runtime state to be modified.
 */
void server_reset_configuration(client_state_t *client_state);

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
 * - Erases and programs the flash sector
 *
 * @param state_in Pointer to the server_persistent_state_t structure to save.
 */
void __not_in_flash_func(save_server_state)(const server_persistent_state_t *state_in);

/**
 * @brief Retrieves the index of an active client connection matching a flash-stored client.
 *
 * Iterates through active UART server connections and compares their TX pins
 * with the TX pin of the client at the specified index in the persistent flash state.
 *
 * @param flash_client_index Index of the client in the persistent flash state array.
 * @param state The loaded persistent server state.
 * @return Index of the matching active connection, or INVALID_CLIENT_INDEX if not found.
 */
uint32_t get_active_client_connection_index_from_flash_client_index(uint32_t flash_client_index, server_persistent_state_t state);

/**
 * @brief Loads saved GPIO states from flash and sends them to active clients.
 *
 * - Verifies CRC of saved state in flash.
 * - Sends current (running) state to each active client over UART.
 * - If CRC is invalid, calls `server_configure_persistent_state()` to reset flash.
 */
void server_load_running_states_to_active_clients(void);

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
 * @brief Checks if a client has any active (ON) devices.
 *
 * Iterates through the client's GPIO-controlled devices and returns true
 * if at least one is currently turned ON.
 *
 * @param client The client structure to inspect.
 * @return true if any device is ON; false otherwise.
 */
bool client_has_active_devices(client_t client);

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

#endif