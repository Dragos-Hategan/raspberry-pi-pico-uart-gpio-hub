#ifndef CLIENT_H
#define CLIENT_H

#include "hardware/uart.h"

#include "config.h"
#include "types.h"

extern uart_connection_t active_uart_client_connection;

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