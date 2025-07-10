/**
 * @file apply_commands.c
 * @brief Handles incoming GPIO commands on the client side.
 *
 * This module:
 * - Listens for UART messages from the server
 * - Parses commands of the form "[gpio, value]"
 * - Applies the commands by controlling GPIO pins
 * - Displays connection info for debugging
 */

#include <stdio.h>

#include "pico/stdlib.h"

#include "client.h"
#include "types.h"
#include "functions.h"

/**
 * @brief Prints the active UART connection info to the console.
 *
 * This function displays the currently connected TX and RX pin pair,
 * along with the UART peripheral number used by the client.
 * 
 * Intended for debugging or informational CLI outputs.
 */
static inline void client_print_active_connection(void){
    // printf("\033[2J");    // delete screen
    // printf("\033[H");     // move cursor to upper left screen
    printf("This is the active connection:\n");
    printf("Pair=[%d,%d]. Instance=uart%d.\n", \
        active_uart_client_connection.pin_pair.tx, \
        active_uart_client_connection.pin_pair.rx, \
        UART_NUM(active_uart_client_connection.uart_instance) \
    ); 
    printf("\n");
}

/**
 * @brief Applies a GPIO command by setting a pin's state.
 *
 * Initializes the given GPIO pin and sets its direction to output,
 * then sets its value to HIGH or LOW based on the second element
 * in the received command array.
 *
 * @param received_number_pair A pointer to a 2-byte array where:
 *        - index 0 = GPIO number
 *        - index 1 = value (0 = LOW, 1 = HIGH)
 */
static void apply_command(uint8_t *received_number_pair){
    gpio_init(received_number_pair[0]);
    gpio_set_dir(received_number_pair[0], GPIO_OUT); 
    gpio_put(received_number_pair[0], received_number_pair[1]);
    printf("[%u,%u]\n", received_number_pair[0], received_number_pair[1]);
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