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
 * @brief Main loop that listens for UART commands and manages power-saving state.
 *
 * Continuously receives data over UART and checks if the client is in a wake-up state.
 * If not, the system enters low-power mode (`dormant`) and waits to be woken up.
 * After waking up, it resumes listening for commands.
 *
 * @note The `waked_up` flag should be managed externally to reflect the wake-up status.
 *
 * @warning This loop runs indefinitely. Ensure that `receive_data()` is non-blocking
 *          or times out appropriately to allow power-saving checks.
 *
 * @see enter_power_saving_mode()
 * @see wake_up()
 * @see receive_data()
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