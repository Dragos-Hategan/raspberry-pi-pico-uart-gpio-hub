/**
 * @file main.c
 * @brief UART client application for Raspberry Pi Pico.
 *
 * This application runs on a Raspberry Pi Pico acting as a UART client.
 * It scans all predefined TX/RX pin combinations for both UART0 and UART1
 * in order to identify a valid UART connection with a server.
 *
 * Once a successful handshake is established, the connection details are stored,
 * and the client begins listening for GPIO control commands from the server.
 */

#include <stdbool.h>

#include "client.h"
#include "functions.h"

/**
 * @brief Main entry point of the UART client program.
 *
 * - Initializes USB stdio and onboard LED.
 * - Repeatedly scans for a UART connection with the server.
 * - Once connected, enters an infinite loop to listen for incoming GPIO control commands.
 *
 * @return Unused (program loops forever).
 */
int main(void){
    init_onboard_led_and_usb();

    while(!client_detect_uart_connection()){
        tight_loop_contents();
    }

    client_listen_for_commands();
}

