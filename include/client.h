#ifndef CLIENT_H
#define CLIENT_H

#include "hardware/uart.h"

#include "config.h"
#include "types.h"

#define CLIENT_TIMEOUT_MS 50

extern uart_connection_t active_uart_client_connection;

bool client_uart_read(uart_inst_t*, uart_pin_pair_t, uint32_t);
bool client_detect_uart_connection();
void client_listen_for_commands();

#endif