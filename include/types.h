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

#endif