/**
 * @file main.c
 * @brief UART client application for Raspberry Pi Pico.
 *
 * This application runs on a Raspberry Pi Pico acting as a UART client.
 * It scans all known TX/RX pin combinations for both UART0 and UART1 to identify a valid
 * UART connection with a server. Once a handshake is successfully established,
 * the client stores the connection for future use.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"
#include "config.h"

/// Timeout in milliseconds used when waiting for UART responses from the server.
#define CLIENT_TIMEOUT_MS 50

static uart_connection_t active_uart_client_connection;

/**
 * @brief Attempts to establish a handshake over a given UART pin pair.
 *
 * This function:
 * - Initializes UART with the specified TX/RX pins.
 * - Sends a connection request message to the server.
 * - Waits for the server to echo the pin pair.
 * - Validates the echo response.
 *
 * @param pin_pair The TX/RX pin pair to test.
 * @param uart_instance Pointer to the UART peripheral (e.g., uart0 or uart1).
 * @return true if the handshake is successful, false otherwise.
 */
bool test_uart_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance) {
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    sleep_ms(10);

    char message_with_pin_pair[32];
    snprintf(message_with_pin_pair, sizeof(message_with_pin_pair), "%s-[%d,%d]", CONNECTION_REQUEST_MESSAGE, pin_pair.tx, pin_pair.rx);
    uart_puts(uart_instance, message_with_pin_pair);
    
    sleep_ms(10);
    return uart_client_read(uart_instance, pin_pair, CLIENT_TIMEOUT_MS);
}

/**
 * @brief Stores the working UART connection in the global state.
 *
 * @param pin_pair The successful TX/RX pin pair.
 * @param uart_instance Pointer to the UART peripheral used.
 */
static inline void add_client_connection(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    active_uart_client_connection.pin_pair = pin_pair;
    active_uart_client_connection.uart_instance = uart_instance;
}

/**
 * @brief Searches UART0 pin pairs for a valid connection with the server.
 *
 * Iterates through all configured UART0 TX/RX combinations, testing each one via `test_uart_pair()`.
 *
 * @return true if a valid UART0 connection is found, false otherwise.
 */
static bool find_connection_for_uart0_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART0_LEN; index++){
        if(test_uart_pair(pin_pairs_uart0[index], uart0)){
            add_client_connection(pin_pairs_uart0[index], uart0);
            return true;
        }else{
            reset_gpio_pins(pin_pairs_uart0[index]);
        }
    }
    return false;
}

/**
 * @brief Searches UART1 pin pairs for a valid connection with the server.
 *
 * Iterates through all configured UART1 TX/RX combinations, testing each one via `test_uart_pair()`.
 *
 * @return true if a valid UART1 connection is found, false otherwise.
 */
static bool find_connection_for_uart1_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART1_LEN; index++){
        if(test_uart_pair(pin_pairs_uart1[index], uart1)){
            add_client_connection(pin_pairs_uart1[index], uart1);
            return true;
        }else{
            reset_gpio_pins(pin_pairs_uart1[index]);
        }
    }
    return false;
}


/**
 * @brief Displays the active UART connection on the console.
 *
 * Shows the connected TX/RX pin pair and the associated UART peripheral number.
 */
static inline void print_active_client_connection(){
    printf("\033[2J");    // delete screen
    printf("\033[H");     // move cursor to upper left screen
    printf("This is the active connection:\n");
    printf("Pair=[%d,%d]. Instance=uart%d.\n", \
        active_uart_client_connection.pin_pair.tx, \
        active_uart_client_connection.pin_pair.rx, \
        UART_NUM(active_uart_client_connection.uart_instance) \
    ); 
    printf("\n\n");
}

/**
 * @brief Performs a full scan of all available UART pin pairs until a valid connection is found.
 *
 * Tries all UART0 and UART1 pin pair combinations. Once a working connection is found,
 * the onboard LED blinks to signal success.
 *
 * @return true if a valid connection is found, false otherwise.
 */
static bool detect_uart_connection(){
    bool connection_found = false;
    connection_found = find_connection_for_uart0_instance();
    if (!connection_found){
        connection_found = find_connection_for_uart1_instance();
    }
    if (connection_found){
        blink_onboard_led();
    }

    return connection_found;
}

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

    while(!detect_uart_connection()){
        tight_loop_contents();
    }
    
    while(true){
        print_active_client_connection();
        sleep_ms(1000);
    }
    
    //while(true){tight_loop_contents();}
}

