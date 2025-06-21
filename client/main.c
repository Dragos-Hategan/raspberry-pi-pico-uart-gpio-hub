#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"

#define TIMEOUT_MS 100

bool test_uart_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance) {
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    uart_puts(uart_instance, CONNECTION_REQUEST_MESSAGE);
    return uart_read_until_match(uart_instance, CONNECTION_ACCEPTED_MESSAGE, TIMEOUT_MS);
}

static bool find_connection_for_uart0_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART0_LEN; index++){
        if(test_uart_pair(pin_pairs_uart0[index], uart0)){
            return true;
        }
    }
    return false;
}

static bool find_connection_for_uart1_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART1_LEN; index++){
        if(test_uart_pair(pin_pairs_uart1[index], uart1)){
            return true;
        }
    }
    return false;
}

static void detect_uart_connection(){
    bool connection_found = false;
    while (!connection_found){
        connection_found = find_connection_for_uart0_instance();
        if (!connection_found){
            connection_found = find_connection_for_uart1_instance();
        }
    }

    if (connection_found){
        blink_onboard_led();
    }
}

int main(){
    detect_uart_connection();

    while(true){tight_loop_contents();}
}

