#include <stdio.h>
#include <string.h>

#include "functions.h"
#include "config.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/**
 * @brief Initializes the onboard LED, depending on available hardware.
 *
 * - If using CYW43 Wi-Fi chip: initializes via `cyw43_arch_init(void)`.
 * - If using default GPIO LED: configures the pin as output.
 *
 * @return int Returns `PICO_OK` on success, or `-1` if no LED config is found.
 */
int pico_led_init(void) {
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

/**
 * @brief Turns the onboard LED on or off.
 * 
 * @param led_on true to turn on the LED, false to turn it off
 */
void pico_set_led(bool led_on) {
#if defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);    
#endif
}

/**
 * @brief Blinks the onboard LED 5 times with a defined delay.
 */
void blink_onboard_led(void){
    int blink_iterations = 5;
    while (blink_iterations--){
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
    }
}

/**
 * @brief Reads incoming UART data into a buffer with timeout support.
 *
 * This function reads characters from the UART until:
 * - The buffer is full,
 * - The character ']' is received,
 * - Or the timeout expires.
 *
 * @param uart UART instance to read from.
 * @param buf Pointer to the buffer to store the received characters.
 * @param buffer_size Size of the buffer (must include space for null terminator).
 * @param timeout_ms Timeout duration in milliseconds.
 */
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

/**
 * @brief Extracts two decimal numbers from a string of the form "[x,y]".
 *
 * This function scans the buffer and parses two integers separated by a comma.
 * The numbers are written into the provided array.
 *
 * @param received_number_pair Pointer to a 2-element array to hold the parsed numbers.
 * @param buf Input buffer containing the formatted string.
 */
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

/**
 * @brief Initializes UART and configures TX/RX GPIO pins.
 *
 * - Deinitializes the UART first to reset state.
 * - Assigns UART function to the specified TX and RX pins.
 * - Initializes the UART with the provided baud rate.
 *
 * @param uart UART instance (e.g., uart0 or uart1).
 * @param pin_pair The TX/RX pin pair to configure.
 * @param baudrate UART baud rate (e.g., 9600, 115200).
 */
void uart_init_with_pins(uart_inst_t* uart, uart_pin_pair_t pin_pair, uint32_t baudrate){
    uart_deinit(uart);
    gpio_set_function(pin_pair.tx, GPIO_FUNC_UART);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_UART);
    uart_init(uart, baudrate);
}
