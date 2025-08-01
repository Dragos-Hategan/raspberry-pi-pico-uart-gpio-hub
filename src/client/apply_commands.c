/**
 * @file apply_commands.c
 * @brief Handles incoming GPIO commands on the client side.
 *
 * This module:
 * - Listens for UART messages from the server
 * - Parses commands of the form "[gpio, value]"
 * - Applies the commands by controlling GPIO pins
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "client.h"
#include "types.h"
#include "functions.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

static bool go_dormant = false;
static bool value = true;

/**
 * @brief Sets or clears a GPIO pin based on a 2-byte command.
 *
 * Interprets the provided byte pair as:
 * - Byte [0] = GPIO pin number
 * - Byte [1] = Logic level (0 = LOW, 1 = HIGH)
 *
 * Behavior:
 * - If the logic level is `1` (HIGH):  
 *   - Initializes the specified GPIO pin  
 *   - Sets it as output and drives it HIGH
 *
 * - If the logic level is `0` (LOW):  
 *   - Drives the pin LOW  
 *   - Then deinitializes the pin (returns it to high-impedance input)
 *
 * This allows toggling pins **and** releasing unused ones to save power.
 *
 * @param gpio_state_pair Pointer to a 2-byte array: [GPIO number, logic level].
 */
static void change_gpio(uint8_t *gpio_state_pair){
    if (gpio_state_pair[1]){
        gpio_init(gpio_state_pair[0]);
        gpio_set_dir(gpio_state_pair[0], GPIO_OUT); 
        gpio_put(gpio_state_pair[0], gpio_state_pair[1]);
    }else{
        gpio_put(gpio_state_pair[0], gpio_state_pair[1]);
        gpio_deinit(gpio_state_pair[0]);
    }
}

/**
 * @brief Enters low-power dormant mode using ROSC as the clock source.
 *
 * Configures the system to use the ROSC oscillator, then enters dormant mode.
 * The system will remain in dormant state until a high level is detected
 * on the RX pin of the active UART connection.
 *
 * @note Assumes `active_uart_client_connection` is correctly initialized.
 *       The RX pin is used as the wake-up source.
 *
 * @warning GPIO configuration for the wake-up pin must be compatible with
 *          level-high wake detection, or the system may fail to resume.
 *
 * @see sleep_run_from_dormant_source()
 * @see sleep_goto_dormant_until_pin()
 */
static void enter_power_saving_mode(){
    //reset_gpio_pins(active_uart_client_connection.pin_pair);
    sleep_run_from_dormant_source(DORMANT_SOURCE_ROSC);
    sleep_goto_dormant_until_pin(active_uart_client_connection.pin_pair.rx, IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_HIGH_BITS, true);
}

/**
 * @brief Wakes up the system from dormant mode and reinitializes UART.
 *
 * Calls `sleep_power_up()` to restore clocks and peripherals,
 * then reinitializes the UART with the previously stored configuration,
 * including UART instance, TX/RX pins, and default baud rate.
 *
 * @return Always returns `true` (currently unused).
 *
 * @note Assumes that `active_uart_client_connection` is valid and preconfigured.
 *
 * @see sleep_power_up()
 * @see uart_init_with_pins()
 */
static bool wake_up(){
    sleep_power_up();

    uart_init_with_pins(active_uart_client_connection.uart_instance,
            active_uart_client_connection.pin_pair,
            DEFAULT_BAUDRATE
    );
}

/**
 * @brief Applies a command based on a received UART message.
 *
 * Interprets the first byte of the received number pair as a command flag
 * and performs the corresponding action, such as resetting the device,
 * blinking the onboard LED, toggling the power state, or changing GPIO state.
 *
 * @param received_number_pair Pointer to an array of two `uint8_t` values.
 *        The first element is interpreted as a command flag.
 *
 * Supported command flags:
 * - `TRIGGER_RESET_FLAG_NUMBER` → Soft reset using watchdog
 * - `BLINK_ONBOARD_LED_FLAG_NUMBER` → Blink onboard LED (blocking)
 * - `WAKE_UP_FLAG_NUMBER` → Set `go_dormant = false`
 * - `DORMANT_FLAG_NUMBER` → Set `go_dormant = true`
 * - Any other value → Delegated to `change_gpio()`
 *
 * @note This function includes debug output via `printf()` for logging purposes.
 *
 * @see change_gpio()
 * @see watchdog_reboot()
 * @see fast_blink_onboard_led_blocking()
 */
static void apply_command(uint8_t *received_number_pair){
    switch(received_number_pair[0]){
        case TRIGGER_RESET_FLAG_NUMBER: watchdog_reboot(0, 0, 0); break;
        case BLINK_ONBOARD_LED_FLAG_NUMBER: fast_blink_onboard_led_blocking(); printf("periodic blink!\n"); break;
        case WAKE_UP_FLAG_NUMBER: go_dormant = false; printf("woke up!\n"); break;//pico_set_onboard_led(value); value = !value; break;
        case DORMANT_FLAG_NUMBER: go_dormant = true; printf("getting dormant!\n"); break;

        default: change_gpio(received_number_pair); printf("gpio change!\n"); break;
    }
}

/**
 * @brief Receives and processes a UART command.
 *
 * This function attempts to read a UART message into a buffer and parse it
 * into a numeric command. If the buffer is not empty, it applies the
 * corresponding command using the parsed number pair.
 *
 * @return true if a valid command was received and processed, false otherwise.
 */
static bool receive_data(){
    char buf[8] = {0};
    uint8_t received_number_pair[2] = {0};

    get_uart_buffer(active_uart_client_connection.uart_instance, buf, sizeof(buf), CLIENT_TIMEOUT_MS);
    get_number_pair(received_number_pair, buf);

    if (buf[0] != '\0'){
        apply_command(received_number_pair);
        return 1;
    }

    return 0;
}

void client_listen_for_commands(void){
    while(true){
        receive_data();
        if (go_dormant){
            enter_power_saving_mode();
            wake_up();
        }
    }
}
