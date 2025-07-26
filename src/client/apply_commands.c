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
 * @brief Applies an action to a GPIO pin or handles control flags based on a received byte pair.
 *
 * Interprets the two-byte input and performs one of the following:
 * - If both bytes match `TRIGGER_RESET_FLAG_NUMBER`, triggers a full system reset via watchdog.
 * - If both bytes match `BLINK_ONBOARD_LED_FLAG_NUMBER`, performs a fast onboard LED blink.
 * - Otherwise, treats the bytes as:
 *    - [0] = GPIO pin number
 *    - [1] = Logic level (0 = LOW, 1 = HIGH)
 *   and applies the command by setting the pin direction and value.
 *
 * @param received_number_pair Pointer to a 2-byte array containing the command.
 */
static void apply_command(uint8_t *received_number_pair){
    if (received_number_pair[0] == TRIGGER_RESET_FLAG_NUMBER && received_number_pair[1] == TRIGGER_RESET_FLAG_NUMBER){
        watchdog_reboot(0, 0, 0);
    }else if (received_number_pair[0] == BLINK_ONBOARD_LED_FLAG_NUMBER && received_number_pair[1] == BLINK_ONBOARD_LED_FLAG_NUMBER){
        fast_blink_onboard_led();
    }else{
        gpio_init(received_number_pair[0]);
        gpio_set_dir(received_number_pair[0], GPIO_OUT); 
        gpio_put(received_number_pair[0], received_number_pair[1]);
    }
}

void client_listen_for_commands(void){
    while(true){
        char buf[8] = {0};
        uint8_t received_number_pair[2] = {0};

        get_uart_buffer(active_uart_client_connection.uart_instance, buf, sizeof(buf), CLIENT_TIMEOUT_MS);
        get_number_pair(received_number_pair, buf);

        if (buf[0] != '\0'){
            apply_command(received_number_pair);
        }
    }
}