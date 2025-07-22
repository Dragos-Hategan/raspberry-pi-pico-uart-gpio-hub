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
#include <stdio.h>

#include "client.h"
#include "functions.h"

absolute_time_t t_start;
absolute_time_t t_stop;
int64_t timp_stdio_usb_init;

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
    stdio_usb_init();
    sleep_ms(2000);

    t_start = get_absolute_time();
    pico_led_init();
    t_stop = get_absolute_time();
    timp_stdio_usb_init = absolute_time_diff_us(t_start, t_stop);
    //printf("pico_led_init() lasts %lldus\n", timp_stdio_usb_init);

    t_start = get_absolute_time();
    pico_set_led(true);
    t_stop = get_absolute_time();
    timp_stdio_usb_init = absolute_time_diff_us(t_start, t_stop);
    //printf("\npico_set_led() lasts %lldus\n\n", timp_stdio_usb_init);

    t_start = get_absolute_time();
    while(!client_detect_uart_connection()){
        tight_loop_contents();
    }
    t_stop = get_absolute_time();
    timp_stdio_usb_init = absolute_time_diff_us(t_start, t_stop);
    //printf("client_detect_uart_connection() loop lasts %lldus\n", timp_stdio_usb_init);

    //sleep_ms(10000);

    client_listen_for_commands();
}

