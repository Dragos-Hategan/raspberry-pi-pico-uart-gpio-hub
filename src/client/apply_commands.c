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
 * @brief Applies a GPIO command based on a received byte pair.
 *
 * If the byte pair matches the reset trigger, a watchdog reset is initiated.
 * Otherwise, the function initializes the target GPIO pin, sets it as output,
 * and writes the desired value.
 *
 * @param received_number_pair A pointer to a 2-byte array:
 *        - [0] = GPIO number
 *        - [1] = Value (0 = LOW, 1 = HIGH)
 */
static void apply_command(uint8_t *received_number_pair){
    if (received_number_pair[0] == TRIGGER_RESET_FLAG_NUMBER && received_number_pair[1] == TRIGGER_RESET_FLAG_NUMBER){
        watchdog_reboot(0, 0, 0);
    }

    gpio_init(received_number_pair[0]);
    gpio_set_dir(received_number_pair[0], GPIO_OUT); 
    gpio_put(received_number_pair[0], received_number_pair[1]);
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