#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"

#define TIMEOUT_MS 250

typedef struct{
    uart_pin_pair_t pin_pair;
    uart_inst_t* uart_instance;
}active_uart_connections_t;

active_uart_connections_t active_uart_connections[MAX_CONNECTIONS];
uint8_t active_connections_number = 0;

static inline bool check_pin_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    return uart_read_until_match(uart_instance, CONNECTION_REQUEST_MESSAGE, TIMEOUT_MS);
}

static inline void add_active_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    if (active_connections_number < MAX_CONNECTIONS) {
        active_uart_connections[active_connections_number].pin_pair = pin_pair;
        active_uart_connections[active_connections_number].uart_instance = uart_instance;
        active_connections_number++;
    }
}

static void check_connections_for_uart0_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART0_LEN; index++){
        if(check_pin_pair(pin_pairs_uart0[index], uart0)){
            uart_puts(uart0, CONNECTION_ACCEPTED_MESSAGE);
            add_active_pair(pin_pairs_uart0[index], uart0);
        }
    }
}

static void check_connections_for_uart1_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART1_LEN; index++){
        if(check_pin_pair(pin_pairs_uart1[index], uart1)){
            add_active_pair(pin_pairs_uart1[index], uart1);
        }
    }
}

static void find_connections(){
    check_connections_for_uart0_instance();
    check_connections_for_uart1_instance();
}

int main(){
    find_connections();

    while(true){tight_loop_contents();}
}
