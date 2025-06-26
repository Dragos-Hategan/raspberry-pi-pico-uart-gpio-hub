#ifndef TYPES_H
#define TYPES_H

#include "hardware/uart.h"
#include "config.h"

typedef struct{
    int tx;
    int rx;
}uart_pin_pair_t;

extern const uart_pin_pair_t pin_pairs_uart0[];
extern const uart_pin_pair_t pin_pairs_uart1[];

// Can add connection_status, latency, etc... for dinamism.
typedef struct{
    uart_pin_pair_t pin_pair;
    uart_inst_t* uart_instance;
}uart_connection_t;

#endif