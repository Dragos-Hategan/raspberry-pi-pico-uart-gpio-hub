/**
 * @file main.c
 * @brief UART server application for Raspberry Pi Pico.
 *
 * This program runs on a Raspberry Pi Pico acting as a UART server.
 * It scans UART0 and UART1 pin pairs to detect connection requests from clients.
 * When a valid TX/RX pair is identified and a handshake is successfully completed,
 * the server stores the connection for further communication.
 */

#include <stdbool.h>

#include "functions.h"
#include "server.h"
#include "menu.h"

/**
 * @brief Main entry point of the UART server application.
 *
 * Initializes standard USB output and the onboard LED.
 * Then enters a loop to scan for UART connections.
 * If any connections are found, a visual LED indicator is triggered and
 * the program continuously prints the active connection list.
 */
int main(void){
    stdio_usb_init();
    pico_led_init();
    pico_set_led(true);

    bool connections_found = false;
    while(!connections_found){
        connections_found = server_find_connections();
    }
    
    if (connections_found){
        blink_onboard_led();
        server_load_running_states_to_active_clients();
    }

    while(true){
        if (stdio_usb_connected()){
            server_display_menu();
        }else{
            tight_loop_contents();
        }
    }
}
