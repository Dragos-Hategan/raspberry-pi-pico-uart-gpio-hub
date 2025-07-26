/**
 * @file functions.c
 * @brief Common utility functions for UART communication and LED control.
 *
 * This file contains shared helper functions used across the project,
 * such as UART buffer parsing, LED signaling, and UART initialization.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdio_usb.h"

#include "functions.h"
#include "config.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

void uart_init_with_pins(uart_inst_t* uart, uart_pin_pair_t pin_pair, uint32_t baudrate){
    uart_deinit(uart);
    gpio_set_function(pin_pair.tx, GPIO_FUNC_UART);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_UART);
    uart_init(uart, baudrate);
}

void get_number_pair(uint8_t *received_number_pair, char *buf){
    char *p = buf;
    uint8_t number_pair_array_index = 0;

    while (*p) {
        if (*p - '0' >= 0 && *p - '0' <= 9) {
            received_number_pair[number_pair_array_index] = received_number_pair[number_pair_array_index] * 10 + (*p - '0');
        } else if (*p == ',') {
            number_pair_array_index++;
        }
        p++;
    }
}

void get_uart_buffer(uart_inst_t* uart, char* buf, uint8_t buffer_size, uint32_t timeout_ms) {
    absolute_time_t start_time = get_absolute_time();
    uint8_t idx = 0;
    uint32_t timeout_us = timeout_ms * MS_TO_US_MULTIPLIER;

    if (uart_is_readable(uart)) {
        char c = uart_getc(uart);
    }

    while (absolute_time_diff_us(start_time, get_absolute_time()) < timeout_us) {
        if (uart_is_readable(uart)) {
            char c = uart_getc(uart);
            if (idx < buffer_size - 1) {
                buf[idx++] = c;
            } 
            else {
                break;
            }

            if (c == ']'){
                break;
            } 
        }
    }

    buf[idx] = '\0';  
}

int pico_onboard_led_init(void) {
#if defined(CYW43_WL_GPIO_LED_PIN)
    return cyw43_arch_init();
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#else
    return -1;
#endif
}

void pico_set_onboard_led(bool led_on) {
#if defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);    
#endif
}

void blink_onboard_led(void){
    int blink_iterations = 5;
    while (blink_iterations--){
        pico_set_onboard_led(false);
        sleep_ms(LED_DELAY_MS);
        pico_set_onboard_led(true);
        sleep_ms(LED_DELAY_MS);
    }
    pico_set_onboard_led(false);
}

void fast_blink_onboard_led(void){
    pico_set_onboard_led(true);
    sleep_ms(FAST_LED_DELAY_MS);
    pico_set_onboard_led(false);
}

void init_onboard_led_and_usb(void){
    pico_onboard_led_init();
    pico_set_onboard_led(true);    
    stdio_usb_init();
}
