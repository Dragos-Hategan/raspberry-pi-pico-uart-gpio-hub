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
 * @brief Initializes the onboard LED, depending on available hardware.
 *
 * - If using CYW43 Wi-Fi chip: initializes via `cyw43_arch_init(void)`.
 * - If using default GPIO LED: configures the pin as output.
 *
 * @return int Returns `PICO_OK` on success, or `-1` if no LED config is found.
 */
int pico_onboard_led_init(void);

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


extern absolute_time_t t_start;
extern absolute_time_t t_stop;
extern absolute_time_t t_start2;
extern absolute_time_t t_stop2;

typedef enum {
    DORMANT_SOURCE_NONE,
    DORMANT_SOURCE_XOSC,
    DORMANT_SOURCE_ROSC,
    DORMANT_SOURCE_LPOSC, // rp2350 only
} dormant_source_t;

/**
 * @brief Prepares the system clocks for low-power dormant wake-up and reinitializes UART.
 *
 * Configures the clock sources and stops unused clocks (USB, ADC, etc.) to allow
 * the RP2040 to safely enter and resume from dormant mode. Selects a low-power 
 * clock source (XOSC or ROSC), disables the unused oscillator, and reinitializes
 * the default UART with the new clock configuration.
 *
 * @param dormant_source The clock source to be used during and after dormant mode.
 *                       Supported values:
 *                       - DORMANT_SOURCE_XOSC (external crystal oscillator)
 *                       - DORMANT_SOURCE_ROSC (ring oscillator)
 *                       - DORMANT_SOURCE_LPOSC (low-power oscillator, if available)
 *
 * @note This function updates global `_dormant_source`, disables PLLs, and stops
 *       clocks not needed in low-power state. It must be called **before** entering dormant mode.
 *
 * @warning The function assumes both XOSC and ROSC are active initially. It will
 *          disable the unused oscillator depending on the selected source.
 *
 * @see setup_default_uart()
 */
void sleep_run_from_dormant_source(dormant_source_t dormant_source);

/**
 * @brief Puts the system into dormant mode until a specified GPIO pin triggers a wake-up event.
 *
 * Configures a GPIO pin as the wake-up source using either edge or level detection,
 * then enters dormant mode. Execution resumes only when the pin event occurs.
 * After wake-up, the interrupt is acknowledged and the pin is deactivated.
 *
 * @param gpio_pin The GPIO number to monitor (must be < NUM_BANK0_GPIOS).
 * @param edge     If true, the wake-up will occur on edge detection (rising/falling).
 *                 If false, level detection (high/low) is used instead.
 * @param high     Determines the polarity:
 *                 - If true: rising edge or high level
 *                 - If false: falling edge or low level
 *
 * @note This function blocks until the wake-up event is triggered on the specified pin.
 *
 * @warning The pin must not be driven with unstable signals during dormant mode,
 *          or false wake-ups may occur.
 *
 * @see gpio_set_dormant_irq_enabled()
 * @see _go_dormant()
 */
void sleep_goto_dormant_until_pin(uint gpio_pin, bool edge, bool high);

/**
 * @brief Restores system clocks and peripherals after wake-up from dormant mode.
 *
 * Re-enables the ring oscillator (ROSC), resets the sleep enable registers,
 * reinitializes all clock domains, and restores the UART for standard output.
 * On RP2350, also reconfigures the power management timer to use XOSC as source.
 *
 * @note This function must be called after waking from dormant mode to restore
 *       full functionality of peripherals and system clocks.
 *
 * @warning Failure to call this function after wake-up may result in malfunctioning
 *          peripherals or missing UART output.
 *
 * @see clocks_init()
 * @see setup_default_uart()
 */
void sleep_power_up(void);

#endif
