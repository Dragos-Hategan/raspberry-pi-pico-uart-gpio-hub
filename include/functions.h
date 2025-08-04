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
void uart_init_with_pins(uart_inst_t *uart, uart_pin_pair_t pin_pair, uint32_t baud_rate);

/**
 * @brief Initializes a UART peripheral on a specific pin.
 *
 * Deinitializes the UART, sets the pin function to UART, and reinitializes
 * the UART with the given baudrate. Waits 1 ms for stability.
 *
 * @param uart Pointer to the UART instance (e.g., uart0 or uart1).
 * @param pin_number GPIO pin to configure for UART.
 * @param baudrate Baudrate for UART communication.
 */
void uart_init_with_single_pin(uart_inst_t* uart, uint8_t pin_number, uint32_t baudrate);

/**
 * @brief Extracts a [tx,rx] pair from a UART message.
 *
 * Parses strings in the format "[tx,rx]" and stores the result in a byte array.
 *
 * @param result_array Pointer to array of 2 bytes to store TX and RX values.
 * @param message Input string containing the UART message (e.g., "[4,5]").
 */
void get_number_pair(uint8_t *result_array, char *message);

/**
 * @brief Reads UART data into a buffer until ']' or timeout.
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
void get_uart_buffer(uart_inst_t *uart, char *buffer, uint8_t buffer_size, uint32_t timeout_ms);

/**
 * @brief Turns the onboard LED on or off.
 * 
 * @param led_on true to turn on the LED, false to turn it off
 */
void pico_set_onboard_led(bool state);

/**
 * @brief Blinks the onboard LED 5 times with a defined delay.
 */
void blink_onboard_led_blocking(void);

/**
 * @brief Performs a quick blink of the onboard LED.
 *
 * Turns the onboard LED on and sets a timer for turning it off(`FAST_LED_DELAY_MS`).
 * Used as a visual indicator for events like client pings.
 */
void fast_blink_onboard_led(void);

/**
 * @brief Performs a quick blink of the onboard LED.
 *
 * Turns the onboard LED on, waits for a short delay (`FAST_LED_DELAY_MS`),
 * then turns it off. Used as a visual indicator for events like client pings.
 */
void fast_blink_onboard_led_blocking(void);

/**
 * @brief Resets the TX and RX pins to default SIO mode.
 *
 * Used to return the pins to GPIO after UART communication is used.
 *
 * @param pin_pair TX/RX pin pair to reset.
 */
static inline void reset_gpio_pins(uart_pin_pair_t pin_pair){
    gpio_set_function(pin_pair.tx, GPIO_FUNC_SIO);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_SIO);
}

/**
 * @brief Initializes onboard LED and USB stdio interface.
 *
 * Turns on the onboard LED and sets up the USB serial connection.
 */
void init_onboard_led_and_usb(void);

#endif
