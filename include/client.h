#ifndef CLIENT_H
#define CLIENT_H

#include "hardware/uart.h"

#include "config.h"
#include "types.h"

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
 * @brief Continuously listens for incoming UART GPIO commands.
 *
 * This function blocks in a loop, waiting for `[gpio, value]` messages
 * over the active UART connection. It parses and applies the received
 * command by toggling the corresponding GPIO pin.
 *
 * Timeout is handled internally via `CLIENT_TIMEOUT_MS`.
 */
void client_listen_for_commands(void);

#endif