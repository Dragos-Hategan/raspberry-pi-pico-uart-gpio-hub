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
#include "hardware/sync.h"
#include "pico/multicore.h"

#include "client.h"
#include "types.h"
#include "functions.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/**
 * @brief Sets or clears a GPIO pin based on a number and logic level.
 *
 * Behavior:
 * - If logic level is `1` (HIGH):
 *   - Initializes the specified GPIO pin
 *   - Sets it as output and drives it HIGH
 *
 * - If logic level is `0` (LOW):
 *   - Drives the pin LOW
 *   - Then deinitializes the pin (returns to high-impedance input)
 *
 * This allows toggling pins **and** releasing unused ones to save power.
 *
 * @param gpio_number GPIO pin number (0–22 and 26-28).
 * @param gpio_state  Logic level: 0 = LOW, 1 = HIGH.
 */
static void change_gpio(uint8_t gpio_number, uint8_t gpio_state){
    if (gpio_state){
        gpio_init(gpio_number);
        gpio_set_dir(gpio_number, GPIO_OUT); 
        gpio_put(gpio_number, gpio_state);
    }else{
        gpio_put(gpio_number, gpio_state);
        gpio_deinit(gpio_number);
    }
}

/**
 * @brief Applies a command based on a received UART message.
 *
 * Interprets the first byte of the received number pair as a command flag
 * and performs the corresponding action, such as resetting the device,
 * blinking the onboard LED, toggling the power state, or changing GPIO state.
 *
 * @param received_number_pair Pointer to a read-only array of two `uint8_t` values.
 *        The first element is interpreted as a command flag.
 *
 * Supported command flags:
 * - `TRIGGER_RESET_FLAG_NUMBER` → Soft reset using watchdog
 * - `BLINK_ONBOARD_LED_FLAG_NUMBER` → Blink onboard LED (blocking)
 * - `WAKE_UP_FLAG_NUMBER` → Set `go_dormant_flag = false`
 * - `DORMANT_FLAG_NUMBER` → Set `go_dormant_flag = true`
 * - Any other value → Delegated to `change_gpio()`
 *
 * @note This function includes debug output via `printf()` for logging purposes.
 *
 * @see change_gpio()
 * @see watchdog_reboot()
 * @see fast_blink_onboard_led_blocking()
 */
static void apply_command(const uint8_t *received_number_pair){
    uint8_t number1 = received_number_pair[0];
    uint8_t number2 = received_number_pair[1];

    switch(number1){
        case TRIGGER_RESET_FLAG_NUMBER: watchdog_reboot(0, 0, 0);
            break;
        case BLINK_ONBOARD_LED_FLAG_NUMBER: fast_blink_onboard_led_blocking(); // go_dormant_flag ? fast_blink_onboard_led_blocking() : fast_blink_onboard_led();
            break;
        case WAKE_UP_FLAG_NUMBER: go_dormant_flag = false;
            break;
        case DORMANT_FLAG_NUMBER: go_dormant_flag = true;
            break;

        default: 
            if ((0 <= number1 && 22 >= number1) || (26 <= number1 && 28 >= number1))
                change_gpio(number1, number2);
            break;
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
static bool receive_data(void){
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
    //uart_set_irq_enables(active_uart_client_connection.uart_instance, true, false);
    
    while(true){
        receive_data();
        if (go_dormant_flag){
            enter_power_saving_mode();
            wake_up();
            //client_turn_off_unused_power_consumers();
        }else{
            //__wfi();
        }
    }
}
