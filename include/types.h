/**
 * @file types.h
 * @brief Defines core data structures used for UART communication and device control.
 *
 * Includes definitions for UART pin pairs, connection objects, device state,
 * and client configurations used on both the client and server side.
 */

#ifndef TYPES_H
#define TYPES_H

#include "hardware/uart.h"

#include "config.h"

/**
 * @brief Represents a UART TX/RX pin pair.
 *
 * Used for defining which GPIO pins are mapped to UART transmit and receive lines.
 */
typedef struct{
    uint8_t tx;
    uint8_t rx;
}uart_pin_pair_t;

/**
 * @brief UART0 pin pairs used for scanning possible connections.
 *
 * These are the valid pin configurations that support UART0 hardware on RP2040.
 */
extern const uart_pin_pair_t pin_pairs_uart0[];

/**
 * @brief UART1 pin pairs used for scanning possible connections.
 *
 * These are the valid pin configurations that support UART1 hardware on RP2040.
 */
extern const uart_pin_pair_t pin_pairs_uart1[];

/**
 * @brief Represents an established UART connection.
 *
 * Used by clients and the server to store an active UART peripheral
 * and associated TX/RX pins.
 */
typedef struct{
    uart_pin_pair_t pin_pair;
    uart_inst_t* uart_instance;
}uart_connection_t;

/**
 * @brief Represents an active UART connection detected by the server.
 *
 * Also stores the reverse TX/RX pair used by the client to send data to the server.
 */
typedef struct{
    uart_pin_pair_t pin_pair;
    uart_inst_t* uart_instance;
    uart_pin_pair_t uart_pin_pair_from_client_to_server; ///< Reverse pin mapping from client
}server_uart_connection_t;

/**
 * @brief Represents a single controllable GPIO device on a client.
 *
 * Each device corresponds to a GPIO number and an ON/OFF state.
 */
typedef struct{
    uint8_t gpio_number;
    bool is_on;
}device_t;

/**
 * @brief Represents the current or saved state of a client's GPIO devices.
 *
 * Includes the status of all available GPIOs (usually 26 per client).
 */
typedef struct{
    device_t devices[MAX_NUMBER_OF_GPIOS];  ///< Array of GPIO devices
}client_state_t;

/**
 * @brief Represents a complete client entry in the system.
 *
 * Contains both live (running) GPIO state and a number of preset configurations.
 * Also stores the UART connection information and whether the client is active.
 */
typedef struct{
    client_state_t running_client_state;
    client_state_t preset_configs[NUMBER_OF_POSSIBLE_PRESETS];
    uart_connection_t uart_connection;
    bool is_active;
}client_t;

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
 * @struct input_client_data_t
 * @brief Stores all user-selected input values required for client-related operations.
 *
 * This structure holds temporary values gathered via CLI interaction and is used
 * to perform operations such as:
 * - Selecting a client (from runtime or flash)
 * - Choosing a device (GPIO)
 * - Setting a desired GPIO state (ON/OFF/TOGGLE)
 * - Selecting a configuration preset
 * - Selecting a reset type (e.g., running, preset, all)
 *
 * It also includes pointers to the current runtime client state and the persistent server state.
 */
typedef struct{
    uint32_t client_index;
    uint32_t flash_client_index;
    uint32_t device_index;
    uint32_t device_state;
    uint32_t flash_configuration_index;
    uint32_t reset_choice;
    const client_state_t *client_state;
    const server_persistent_state_t *flash_state;
}input_client_data_t;

/**
 * @struct client_input_flags_t
 * @brief Specifies which user inputs should be collected when interacting with a client.
 *
 * This structure allows modular control over the type of input required for different operations.
 * It enables the reuse of a common input collection function (`read_client_data`) across various
 * CLI features such as toggling GPIOs, building presets, or resetting configurations.
 */
typedef struct {
    bool need_client_index;
    bool need_device_index;
    bool need_device_state;
    bool need_config_index;
    bool is_building_preset;
    bool need_reset_choice;
    bool is_load;
}client_input_flags_t;

#endif