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

#include "hardware/timer.h"

#include "functions.h"
#include "server.h"
#include "menu.h"

absolute_time_t t_start;
absolute_time_t t_stop;
int64_t timp_stdio_usb_init;

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
    t_start = get_absolute_time(); //get_absolute_time();
    stdio_usb_init();
    t_stop = get_absolute_time(); //get_absolute_time();
    timp_stdio_usb_init = absolute_time_diff_us(t_start, t_stop);
    //printf("stdio_usb_init() lasts %lldus\n", timp_stdio_usb_init);
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

    bool connections_found = false;
    while(!connections_found){
        absolute_time_t t_start2 = get_absolute_time();
        connections_found = server_find_connections();
        absolute_time_t t_stop2 = get_absolute_time();
        timp_stdio_usb_init = absolute_time_diff_us(t_start2, t_stop2);
        //printf("server_find_connections() lasts %lldus\n", timp_stdio_usb_init);     

        //sleep_ms(1000);   
        //printf("\n");
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
