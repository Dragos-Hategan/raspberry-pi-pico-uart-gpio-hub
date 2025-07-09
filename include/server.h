#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include "hardware/uart.h"
#include "config.h"
#include "types.h"

typedef struct {
    client_t clients[MAX_SERVER_CONNECTIONS];
    uint32_t crc;
} server_persistent_state_t;

extern server_uart_connection_t active_uart_server_connections[MAX_SERVER_CONNECTIONS];
extern uint8_t active_server_connections_number;

bool server_find_connections(void);
bool server_uart_read(uart_inst_t*, uint32_t);
void server_listen_for_commands(void);
void server_load_running_states_to_active_clients(void);
void server_print_running_client_state(client_t *);
void server_print_client_preset_configuration(client_t *, uint8_t);
void __not_in_flash_func(save_server_state)(const server_persistent_state_t *);
void server_send_client_state(uart_pin_pair_t, uart_inst_t*, const client_state_t*);
void server_send_device_state(uart_pin_pair_t, uart_inst_t*, uint8_t, bool);

inline void server_print_gpio_state(uint8_t gpio_index, const client_state_t *client_state){
    if (client_state->devices[gpio_index].gpio_number == UART_CONNECTION_FLAG_NUMBER){
        printf("Device[%u]: UART connection, no access.\n", gpio_index + 1);
    }else{
        printf("Device[%u]: gpio_number:%u is_on:%u\n", gpio_index + 1, client_state->devices[gpio_index].gpio_number, client_state->devices[gpio_index].is_on);
    }
}

#define SERVER_SECTOR_SIZE    4096
#define SERVER_PAGE_SIZE      256
#define SERVER_FLASH_OFFSET   (PICO_FLASH_SIZE_BYTES - SERVER_SECTOR_SIZE)
#define SERVER_FLASH_ADDR     (XIP_BASE + SERVER_FLASH_OFFSET)

#endif