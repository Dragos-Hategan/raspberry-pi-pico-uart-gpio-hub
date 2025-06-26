#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "types.h"

int pico_led_init(void);
void pico_set_led(bool);
void blink_onboard_led(void);
void uart_init_with_pins(uart_inst_t*, uart_pin_pair_t, uint32_t);
bool uart_client_read(uart_inst_t*, uart_pin_pair_t, uint32_t);
bool uart_server_read(uart_inst_t*, uint32_t);

static inline void reset_gpio_pins(uart_pin_pair_t pin_pair){
    gpio_set_function(pin_pair.tx, GPIO_FUNC_SIO);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_SIO);
}

#endif