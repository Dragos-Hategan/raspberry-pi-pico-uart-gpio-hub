/**
 * @file main.c
 * @brief UART client application for Raspberry Pi Pico.
 *
 * This application runs on a Raspberry Pi Pico acting as a UART client.
 * It scans all known TX/RX pin combinations for both UART0 and UART1 to identify a valid
 * UART connection with a server. Once a handshake is successfully established,
 * the client stores the connection for future use.
 */

#include <stdbool.h>

#include "client.h"
#include "functions.h"

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

    client_listen_for_commands();
}

