#include <stdio.h>

#include "pico/stdlib.h"

#include "client.h"
#include "types.h"
#include "functions.h"

/**
 * @brief Displays the active UART connection on the console.
 *
 * Shows the connected TX/RX pin pair and the associated UART peripheral number.
 */
static inline void client_print_active_connection(){
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

static void apply_command(uint8_t *received_number_pair){
    gpio_init(received_number_pair[0]);
    gpio_set_dir(received_number_pair[0], GPIO_OUT); 
    gpio_put(received_number_pair[0], received_number_pair[1]);
    printf("[%u,%u]\n", received_number_pair[0], received_number_pair[1]);
}

void client_listen_for_commands(){
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