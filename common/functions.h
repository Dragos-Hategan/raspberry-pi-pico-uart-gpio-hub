#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "config.h"

int pico_led_init(void);
void pico_set_led(bool);
void blink_onboard_led(void);
bool uart_read_until_match(uart_inst_t*, const char*, uint);
void uart_init_with_pins(uart_inst_t*, uart_pin_pair_t, uint);

#endif