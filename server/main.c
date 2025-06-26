/**
 * @file main.c
 * @brief UART server application for Raspberry Pi Pico.
 *
 * This program runs on a Raspberry Pi Pico acting as a UART server.
 * It scans UART0 and UART1 pin pairs to detect connection requests from clients.
 * When a valid TX/RX pair is identified and a handshake is successfully completed,
 * the server stores the connection for further communication.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"
#include "config.h"
#include "server.h"

/**
 * @brief Prints all currently active UART connections to the console.
 *
 * Displays each valid UART connection with its associated TX/RX pins and UART instance number.
 */
static inline void print_active_connections(){
    printf("\033[2J");    // delete screen
    printf("\033[H");     // move cursor to upper left screen
    printf("These are the active connections:\n");
    for (uint8_t index = 1; index <= active_server_connections_number; index++){
        printf("%d. Pair=[%d,%d]. Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        }
    printf("\n");
}

/**
 * @brief Main entry point of the UART server application.
 *
 * Initializes standard USB output and the onboard LED.
 * Then enters a loop to scan for UART connections.
 * If any connections are found, a visual LED indicator is triggered and
 * the program continuously prints the active connection list.
 */
int main(){
    stdio_usb_init();
    pico_led_init();
    pico_set_led(true);

    bool connections_found = false;

    while(!connections_found){
        server_find_connections();
        connections_found = true;
    }
    
    if (connections_found){
        blink_onboard_led();
    }

    // TO DO: establish a bidirectional communication protocol with clients

    while(true){
        print_active_connections();
        sleep_ms(1000);
    }

    //while(true){tight_loop_contents();}
}
