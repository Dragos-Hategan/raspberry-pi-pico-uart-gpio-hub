#ifndef SERVER_H
#define SERVER_H

#include "hardware/uart.h"
#include "config.h"
#include "types.h"

extern uart_connection_t active_uart_server_connections[MAX_SERVER_CONNECTIONS];
extern uint8_t active_server_connections_number;

bool server_find_connections();
bool server_uart_read(uart_inst_t*, uint32_t);
void server_listen_for_commands();

typedef struct {
    client_t clients[MAX_SERVER_CONNECTIONS];
    uint32_t crc;
} server_persistent_state_t;

#endif