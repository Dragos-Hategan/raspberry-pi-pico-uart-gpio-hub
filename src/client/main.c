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

#include "client.h"
#include "functions.h"

/**
 * @brief Entry point for the UART client application.
 *
 * Initializes the onboard LED and USB interface, then waits for a valid UART
 * connection with the server. Once connected, the client restores the last
 * known GPIO states and enters the main loop to listen for further commands.
 *
 * @return Unused. This function never returns.
 */
int main(void){
    init_onboard_led_and_usb();

    while(!client_detect_uart_connection()) tight_loop_contents();

    power_saving_config();
    
    client_listen_for_commands();
}

