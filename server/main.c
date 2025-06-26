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

/// Timeout in milliseconds for receiving a UART connection request.
/// Minimum effective value is ~300ms. 500ms provides more robustness.
#define SERVER_TIMEOUT_MS 500

static uart_connection_t active_uart_server_connections[MAX_SERVER_CONNECTIONS];
static uint8_t active_server_connections_number = 0;

/**
 * @brief Initializes UART on the specified pin pair and attempts to detect a connection request.
 *
 * This function is used to check whether a client is attempting to connect on the given TX/RX pin pair.
 *
 * @param pin_pair The TX/RX pin pair to test.
 * @param uart_instance Pointer to the UART peripheral (e.g., uart0 or uart1).
 * @return true if a connection request is successfully detected, false otherwise.
 */
static inline bool check_pin_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    return uart_server_read(uart_instance, SERVER_TIMEOUT_MS);
}

/**
 * @brief Adds a valid UART connection (pin pair + UART instance) to the active connections list.
 *
 * Only adds the connection if there is room in the `active_uart_server_connections` array.
 *
 * @param pin_pair The TX/RX pin pair that was successfully connected.
 * @param uart_instance Pointer to the UART peripheral associated with the connection.
 */
static inline void add_active_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    if (active_server_connections_number < MAX_SERVER_CONNECTIONS) {
        active_uart_server_connections[active_server_connections_number].pin_pair = pin_pair;
        active_uart_server_connections[active_server_connections_number].uart_instance = uart_instance;
        active_server_connections_number++;
    }
}

/**
 * @brief Scans all UART0 pin pairs for active connection requests.
 *
 * For each TX/RX pair configured for UART0, this function attempts to perform a handshake.
 * If the handshake is successful, the connection is stored.
 */
static void check_connections_for_uart0_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART0_LEN; index++){
        if(check_pin_pair(pin_pairs_uart0[index], uart0)){
            add_active_pair(pin_pairs_uart0[index], uart0);
            sleep_ms(20);
        }
        reset_gpio_pins(pin_pairs_uart0[index]);
    }
}

/**
 * @brief Scans all UART1 pin pairs for active connection requests.
 *
 * Similar to UART0 scanning, this function iterates over all UART1 TX/RX pairs
 * and performs handshake attempts.
 */
static void check_connections_for_uart1_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART1_LEN; index++){
        if(check_pin_pair(pin_pairs_uart1[index], uart1)){
            add_active_pair(pin_pairs_uart1[index], uart1);
            sleep_ms(20);
        }
        reset_gpio_pins(pin_pairs_uart1[index]);
    }
}

/**
 * @brief Initiates the scanning process for both UART0 and UART1.
 *
 * Calls the UART0 and UART1 connection checkers to identify all valid connections.
 */
static void find_connections(){
    check_connections_for_uart0_instance();
    check_connections_for_uart1_instance();
}

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
        find_connections();
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
