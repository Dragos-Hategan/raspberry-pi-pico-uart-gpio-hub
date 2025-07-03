#include <stdio.h>
#include "client.h"
#include "types.h"
#include "pico/stdlib.h"

/**
 * @brief Displays the active UART connection on the console.
 *
 * Shows the connected TX/RX pin pair and the associated UART peripheral number.
 */
static inline void client_print_active_connection(){
    printf("\033[2J");    // delete screen
    printf("\033[H");     // move cursor to upper left screen
    printf("This is the active connection:\n");
    printf("Pair=[%d,%d]. Instance=uart%d.\n", \
        active_uart_client_connection.pin_pair.tx, \
        active_uart_client_connection.pin_pair.rx, \
        UART_NUM(active_uart_client_connection.uart_instance) \
    ); 
    printf("\n");
}

void client_listen_for_commands(){
    while(true){
        client_print_active_connection();
        sleep_ms(1000);
    }
}