/**
 * @file main.c
 * @brief UART server application for Raspberry Pi Pico.
 *
 * This program runs on a Raspberry Pi Pico acting as a central UART server.
 * It scans through all possible UART0 and UART1 TX/RX pin pairs to detect
 * connection requests from clients. When a handshake is successfully completed,
 * the server stores the connection in an internal list.
 *
 * It then loads previously saved GPIO states for those clients and exposes
 * a USB CLI interface for real-time control.
 */

#include <stdbool.h>

#include "functions.h"
#include "server.h"
#include "menu.h"

/**
 * @brief Main entry point of the UART server application.
 *
 * Steps:
 * - Initializes USB stdio and onboard LED.
 * - Scans for valid client UART connections via `server_find_connections()`.
 * - Once at least one connection is established:
 *   - Blinks the onboard LED.
 *   - Loads previous GPIO states from internal flash.
 * - Enters a USB CLI loop (if connected).
 *
 * @return int Exit code (not used).
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
