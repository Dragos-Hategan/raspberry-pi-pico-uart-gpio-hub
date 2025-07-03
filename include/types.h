#ifndef TYPES_H
#define TYPES_H

#include "hardware/uart.h"
#include "config.h"

typedef struct{
    uint8_t tx;
    uint8_t rx;
}uart_pin_pair_t;

extern const uart_pin_pair_t pin_pairs_uart0[];
extern const uart_pin_pair_t pin_pairs_uart1[];

typedef struct{
    uart_pin_pair_t pin_pair;
    uart_inst_t* uart_instance;
}uart_connection_t;

typedef struct{
    uint8_t gpio_number;
    bool is_on;
}device_t;

typedef struct{
    device_t devices[MAX_NUMBER_OF_ACTIVE_DEVICES];
}client_state_t;

typedef struct{
    client_state_t running_client_state;
    client_state_t preset_configs[NUMBER_OF_POSSIBLE_PRESETS];
    uart_connection_t uart_connection;
}client_t;

#endif