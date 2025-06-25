/**
 * @file main.c
 * @brief UART server application for Raspberry Pi Pico.
 *
 * Scans UART0 and UART1 pin pairs to detect connection requests from clients.
 * When a valid TX/RX pair is identified based on an expected request message,
 * the server confirms the connection and stores it in a list of active connections.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"
#include "config.h"

#define SERVER_TIMEOUT_MS 250

int point = 0;

static uart_connection_t active_uart_server_connections[MAX_SERVER_CONNECTIONS];
static uint8_t active_server_connections_number = 0;

/**
 * @brief Initializes UART with specified pin pair and checks for a connection request message.
 *
 * Used by the server to identify if a client is attempting to connect.
 *
 * @param pin_pair The TX/RX pin pair to initialize.
 * @param uart_instance UART peripheral instance (uart0 or uart1).
 * @return true if a connection request is detected within SERVER_TIMEOUT_MS, false otherwise.
 */
static inline bool check_pin_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    return uart_server_read(uart_instance, CONNECTION_REQUEST_MESSAGE, SERVER_TIMEOUT_MS);
}

/**
 * @brief Stores a working connection (pin pair + UART instance) into the global list.
 *
 * Only adds if the list is not full.
 *
 * @param pin_pair The TX/RX pair that was successfully tested.
 * @param uart_instance The UART peripheral associated with this connection.
 */
static inline void add_active_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    if (active_server_connections_number < MAX_SERVER_CONNECTIONS) {
        active_uart_server_connections[active_server_connections_number].pin_pair = pin_pair;
        active_uart_server_connections[active_server_connections_number].uart_instance = uart_instance;
        active_server_connections_number++;
    }
}

/**
/**
 * @brief Scans all configured UART0 pin pairs for connection requests.
 *
 * For each pair, attempts a handshake with the client. On success, stores
 * the connection and prints debug output.
 */
static void check_connections_for_uart0_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART0_LEN; index++){
        printf("Checking connections for uart0 instance, INDEX = %d, pin pair {%d,%d}\n", index, pin_pairs_uart0[index].tx, pin_pairs_uart0[index].rx);
        if(check_pin_pair(pin_pairs_uart0[index], uart0)){
            printf("ADDING PAIR [%d,%d]\n", pin_pairs_uart0[index].tx,pin_pairs_uart0[index].rx);
            add_active_pair(pin_pairs_uart0[index], uart0);
        }
        else{
            reset_gpio_pins(pin_pairs_uart0[index]);
        }
    }
}

/**
 * @brief Scans all configured UART1 pin pairs for connection requests.
 *
 * For each pair, attempts a handshake with the client. On success, stores
 * the connection and prints debug output.
 */
static void check_connections_for_uart1_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART1_LEN; index++){
        printf("Checking connections for uart1 instance, INDEX = %d, pin pair {%d,%d}\n", index, pin_pairs_uart1[index].tx, pin_pairs_uart1[index].rx);
        if(check_pin_pair(pin_pairs_uart1[index], uart1)){
            printf("ADDING PAIR [%d,%d]\n", pin_pairs_uart1[index].tx,pin_pairs_uart1[index].rx);
            add_active_pair(pin_pairs_uart1[index], uart1);
        }
        else{
            reset_gpio_pins(pin_pairs_uart1[index]);
        }
    }
}

/**
 * @brief Attempts to find all valid connections across both UART0 and UART1.
 *
 * Calls scanning functions for each UART instance.
 */
static void find_connections(){
    check_connections_for_uart0_instance();
    check_connections_for_uart1_instance();
}

/**
 * @brief Prints all currently active UART connections.
 *
 * Displays the list of successfully connected pin pairs with their corresponding UART instance.
 */
static inline void print_active_connections(){
    //printf("\033[2J");    // delete screen
    //printf("\033[H");     // move cursor to upper left screen
    printf("These are the active connections:\n");
    for (uint8_t index = 1; index <= active_server_connections_number; index++){
        printf("%d. Pair=[%d,%d]. Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        }
    printf("\n\n");
}

/**
 * @brief Entry point of the program.
 *
 * Initializes USB stdio for logging, waits 5 seconds for user setup, then
 * scans for UART connections and continuously prints the results.
 */
int main(){
    stdio_usb_init();
    sleep_ms(10000);
    pico_led_init();
    pico_set_led(true);

    printf("\nStarted server application, entering find_connections()\n");
    find_connections();

    // Create the bidirectional comuncation routine
    // Maybe a small protocol (ex: START_BYTE + LENGTH + CMD + PAYLOAD + CRC)

    if (active_server_connections_number){
        int blink_iterations = 5;
            while (blink_iterations--){
                pico_set_led(true);
                sleep_ms(LED_DELAY_MS);
                pico_set_led(false);
                sleep_ms(LED_DELAY_MS);
            }
    }

    while(true){
        print_active_connections();
        sleep_ms(1000);
    }

    //while(true){tight_loop_contents();}
}
