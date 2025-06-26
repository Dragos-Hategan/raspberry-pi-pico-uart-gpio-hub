/**
 * @file main.c
 * @brief UART client application for Raspberry Pi Pico.
 *
 * Attempts to detect a valid UART connection by scanning through known TX/RX pin pairs.
 * Once a handshake with the server is established, the connection is stored.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"
#include "config.h"

#define CLIENT_TIMEOUT_MS 50

static uart_connection_t active_uart_client_connection;

/**
 * @brief Tests a UART pin pair by attempting a handshake with the server.
 *
 * Sends a predefined connection request message over the specified UART
 * and waits for a valid echo response within the specified timeout.
 *
 * @param pin_pair The TX/RX pin pair to test.
 * @param uart_instance Pointer to the UART peripheral (uart0 or uart1).
 * @return true if a valid response is received, false otherwise.
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
 * @brief Attempts to find a working UART0 pin pair for communication.
 *
 * Iterates through all UART0 pin pairs and tests each using `test_uart_pair()`.
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
 * @brief Attempts to find a working UART1 pin pair for communication.
 *
 * Iterates through all UART1 pin pairs and tests each using `test_uart_pair()`.
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
 * @brief Prints the details of the active UART connection.
 *
 * Displays the successful TX/RX pins and UART instance.
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
 * @brief Scans all possible UART0 and UART1 pin pairs until a working connection is found.
 *
 * Once found, it prints connection info and activates a visual indicator (onboard LED).
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
 * @brief Main entry point of the program.
 *
 * Initializes UART communication detection and enters an infinite loop.
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

