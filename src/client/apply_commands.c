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
 * @brief Executes a command based on a 2-byte input pair.
 *
 * Interprets the first byte of the pair as a command flag:
 *
 * - If it matches `TRIGGER_RESET_FLAG_NUMBER`:  
 *   → Triggers a full system reset via the watchdog.
 *
 * - If it matches `BLINK_ONBOARD_LED_FLAG_NUMBER`:  
 *   → Executes a fast blink on the onboard LED.
 *
 * - Otherwise, treats the pair as a GPIO control command and calls `change_gpio()`, where:
 *   - Byte [0] = GPIO pin number
 *   - Byte [1] = Logic level (0 = LOW, 1 = HIGH)
 *
 * @param received_number_pair Pointer to a 2-byte array representing the command.
 */
static void apply_command(uint8_t *received_number_pair){
    switch(received_number_pair[0]){
        case TRIGGER_RESET_FLAG_NUMBER: watchdog_reboot(0, 0, 0); break;
        case BLINK_ONBOARD_LED_FLAG_NUMBER: fast_blink_onboard_led_blocking(); break;

        default: change_gpio(received_number_pair); break;
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
        //enter_power_saving_mode();
        receive_data();
    }
}

void client_apply_last_running_state(void){
    uint8_t devices_received = 0;

    while(devices_received < MAX_NUMBER_OF_GPIOS){
        if (receive_data()){
            devices_received++;
        }
    }    
}