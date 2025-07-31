/**
 * @file client.h
 * @brief UART client interface for establishing and listening to GPIO commands.
 *
 * This module provides:
 * - UART handshake and connection detection logic (client side)
 * - Receiving and parsing GPIO control commands from server
 * - Global access to the active UART client connection
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "hardware/uart.h"

#include "config.h"
#include "types.h"

/**
 * @brief Global UART connection used by the client.
 *
 * Holds the UART instance and TX/RX pin pair after a successful handshake.
 * Used by client-side modules to send and receive messages.
 */
extern uart_connection_t active_uart_client_connection;

/**
 * @brief Performs a full scan of all available UART pin pairs until a valid connection is found.
 *
 * Tries all UART0 and UART1 pin pair combinations. Once a working connection is found,
 * the onboard LED blinks to signal success.
 *
 * @return true if a valid connection is found, false otherwise.
 */
bool client_detect_uart_connection(void);

/**
 * @brief Continuously listens for UART commands from the server.
 *
 * This function enters an infinite loop where it waits for and processes
 * incoming UART commands from the server using `receive_data()`.
 * Power saving mode is enabled between checks.
 *
 * @note This function blocks indefinitely.
 */
void client_listen_for_commands(void);

/**
 * @brief Applies the last known running state to all client GPIO devices.
 *
 * This function listens for and applies commands over UART to restore
 * the previous state of all GPIO devices. It calls `receive_data()` repeatedly
 * until it has received and processed `MAX_NUMBER_OF_GPIOS` valid commands.
 *
 * @note This function assumes the server sends exactly MAX_NUMBER_OF_GPIOS commands.
 */
void client_apply_last_running_state(void);

#endif