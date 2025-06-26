#ifndef CLIENT_H
#define CLIENT_H

#include "config.h"
#include "hardware/uart.h"

extern uart_connection_t active_uart_client_connection;

bool client_uart_read(uart_inst_t*, uart_pin_pair_t, uint32_t);
bool client_detect_uart_connection();

#endif