/**
 * @file client.h
 * @brief UART client interface for establishing and listening to GPIO commands.
 *
 * This module provides:
 * - UART handshake and connection detection logic (client side)
 * - Receiving and parsing GPIO control commands from server
 * - Global access to the active UART client connection
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "hardware/uart.h"

#include "config.h"
#include "types.h"

/**
 * @brief Global UART connection used by the client.
 *
 * Holds the UART instance and TX/RX pin pair after a successful handshake.
 * Used by client-side modules to send and receive messages.
 */
extern uart_connection_t active_uart_client_connection;

extern bool go_dormant_flag;

/**
 * @brief Performs a full scan of all available UART pin pairs until a valid connection is found.
 *
 * Tries all UART0 and UART1 pin pair combinations. Once a working connection is found,
 * the onboard LED blinks to signal success.
 *
 * @return true if a valid connection is found, false otherwise.
 */
bool client_detect_uart_connection(void);

/**
 * @brief Reduces power consumption by disabling unused clocks and reconfiguring system clocks.
 *
 * This function turns off unnecessary peripherals and clock outputs (e.g., ADC, RTC, GPOUT),
 * switches to lower-frequency XOSC-based system clocks (12 MHz), disables the system PLL,
 * and enables only essential clock domains for sleep mode operation.
 *
 * It also reinitializes the active UART interface with the appropriate pins and baudrate.
 */
void client_turn_off_unused_power_consumers(void);

/**
 * @brief Prepares the system for power saving.
 *
 * Turns off unused components and sets up the wake-up pin for dormant mode.
 *
 * @see client_turn_off_unused_power_consumers()
 * @see set_pin_as_input_for_dormant_wakeup()
 */
void power_saving_config(void);

/**
 * @brief Enters low-power dormant mode using ROSC as the clock source.
 *
 * Configures the system to use the ROSC oscillator, then enters dormant mode.
 * The system will remain in dormant state until a high level is detected
 * on the TX pin of the active UART connection.
 *
 * @note Assumes `active_uart_client_connection` is correctly initialized.
 *       The TX pin is used as the wake-up source.
 *
 * @warning GPIO configuration for the wake-up pin must be compatible with
 *          level-high wake detection, or the system may fail to resume.
 *
 * @see sleep_run_from_dormant_source()
 * @see sleep_goto_dormant_until_pin()
 */
void enter_power_saving_mode(void);

/**
 * @brief Wakes up the system from dormant mode and reinitializes UART.
 *
 * Restores clocks and peripherals using `sleep_power_up()`, then reinitializes
 * UART with stored settings (instance, TX/RX pins, and baud rate).
 *
 * @note Assumes `active_uart_client_connection` is valid.
 *
 * @see sleep_power_up()
 * @see uart_init_with_pins()
 */
void wake_up(void);

/**
 * @brief Main loop that listens for UART commands and manages power-saving state.
 *
 * Continuously receives data over UART and checks if the client is in a wake-up state.
 * If not, the system enters low-power mode (`dormant`) and waits to be woken up.
 * After waking up, it resumes listening for commands.
 *
 * @note The `go_dormant_flag` should be managed externally to reflect the wake-up status.
 *
 * @warning This loop runs indefinitely. Ensure that `receive_data()` is non-blocking
 *          or times out appropriately to allow power-saving checks.
 *
 * @see enter_power_saving_mode()
 * @see wake_up()
 * @see receive_data()
 */
void client_listen_for_commands(void);

#endif