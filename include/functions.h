/**
 * @file functions.h
 * @brief Utility functions for UART, LED control, and GPIO pin parsing.
 *
 * Provides helpers to:
 * - Initialize and blink the onboard LED
 * - Configure UART with specific TX/RX pins
 * - Receive UART data into buffers
 * - Parse UART messages for TX/RX pin pairs
 * - Reset GPIO pins to default SIO mode
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

#include "types.h"

/**
 * @brief Initializes the onboard LED.
 *
 * @return 0 if successful, negative on error.
 */
int pico_led_init(void);

/**
 * @brief Sets the onboard LED state.
 *
 * @param state true to turn on, false to turn off.
 */
void pico_set_led(bool state);

/**
 * @brief Blinks the onboard LED once.
 */
void blink_onboard_led(void);

/**
 * @brief Initializes a UART peripheral with the given TX/RX pins and baud rate.
 *
 * @param uart Pointer to UART instance (uart0 or uart1).
 * @param pin_pair Structure containing TX and RX GPIO pin numbers.
 * @param baud_rate UART communication speed (e.g., 115200).
 */
void uart_init_with_pins(uart_inst_t *uart, uart_pin_pair_t pin_pair, uint32_t baud_rate);

/**
 * @brief Reads a UART message into a character buffer with timeout.
 *
 * Blocks until a message is received or the timeout expires.
 *
 * @param uart Pointer to UART instance.
 * @param buffer Pointer to buffer to store the message.
 * @param buffer_size Size of the buffer (number of characters).
 * @param timeout_ms Timeout in milliseconds.
 */
void get_uart_buffer(uart_inst_t *uart, char *buffer, uint8_t buffer_size, uint32_t timeout_ms);

/**
 * @brief Resets the TX and RX pins to default SIO mode.
 *
 * Used to return the pins to GPIO after UART communication is finished.
 *
 * @param pin_pair TX/RX pin pair to reset.
 */
static inline void reset_gpio_pins(uart_pin_pair_t pin_pair){
    gpio_set_function(pin_pair.tx, GPIO_FUNC_SIO);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_SIO);
}

/**
 * @brief Extracts a [tx,rx] pair from a UART message.
 *
 * Parses strings in the format "[tx,rx]" and stores the result in a byte array.
 *
 * @param result_array Pointer to array of 2 bytes to store TX and RX values.
 * @param message Input string containing the UART message (e.g., "[4,5]").
 */
void get_number_pair(uint8_t *result_array, char *message);

#endif
