#ifndef SERVER_H
#define SERVER_H

#include "hardware/uart.h"
#include "config.h"
#include "types.h"

extern uart_connection_t active_uart_server_connections[MAX_SERVER_CONNECTIONS];
extern uint8_t active_server_connections_number;

void server_find_connections();
bool server_uart_read(uart_inst_t*, uint32_t);

#endif