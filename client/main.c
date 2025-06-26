/**
 * @file main.c
 * @brief UART client application for Raspberry Pi Pico.
 *
 * This application runs on a Raspberry Pi Pico acting as a UART client.
 * It scans all known TX/RX pin combinations for both UART0 and UART1 to identify a valid
 * UART connection with a server. Once a handshake is successfully established,
 * the client stores the connection for future use.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"
#include "config.h"
#include "client.h"

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

/**
 * @brief Main entry point of the UART client program.
 *
 * - Initializes USB stdio and onboard LED.
 * - Enters a loop to detect a valid UART connection.
 * - Once connected, continuously prints connection info.
 */
int main(){
    stdio_usb_init();
    pico_led_init();
    pico_set_led(true);

    while(!client_detect_uart_connection()){
        tight_loop_contents();
    }
    
    while(true){
        client_print_active_connection();
        sleep_ms(1000);
    }
    
    //while(true){tight_loop_contents();}
}

