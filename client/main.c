#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "functions.h"
#include "config.h"

#define TIMEOUT_MS 50

static uart_connection_t active_uart_client_connection;

/**
 * @brief Tests a UART pin pair by attempting a handshake.
 *
 * Sends a predefined connection request message over the specified UART
 * and waits for an expected response within a timeout.
 *
 * @param pin_pair UART TX/RX pin pair to test.
 * @param uart_instance Pointer to the UART instance (uart0 or uart1).
 * @return true if handshake was successful, false otherwise.
 */
bool test_uart_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance) {
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    sleep_ms(10);
    uart_puts(uart_instance, CONNECTION_REQUEST_MESSAGE);
    return uart_read_until_match(uart_instance, CONNECTION_ACCEPTED_MESSAGE, TIMEOUT_MS);
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
        }
    }
    return false;
}


/**
 * @brief Prints the details of the active UART connection.
 *
 * Displays the successful TX/RX pins and UART instance.
 */
static inline void print_active_connection(){
    printf("This is the active connection:\n");

    printf("Pair=[%d,%d]. Instance=uart%d.\n", \
        active_uart_client_connection.pin_pair.tx, \
        active_uart_client_connection.pin_pair.rx, \
        UART_NUM(active_uart_client_connection.uart_instance) \
    ); 

    printf("\n");
}

/**
 * @brief Scans all possible UART0 and UART1 pin pairs until a working connection is found.
 *
 * Once found, it prints connection info and activates a visual indicator (onboard LED).
 */
static void detect_uart_connection(){
    bool connection_found = false;
    while (!connection_found){
        connection_found = find_connection_for_uart0_instance();
        if (!connection_found){
            connection_found = find_connection_for_uart1_instance();
        }
    }

    if (connection_found){
        print_active_connection();
        blink_onboard_led();
    }
}

/**
 * @brief Main entry point of the program.
 *
 * Initializes UART communication detection and enters an infinite loop.
 */
int main(){
    //stdio_usb_init();
    //sleep_ms(10);

    detect_uart_connection();

    while(true){tight_loop_contents();}
}

