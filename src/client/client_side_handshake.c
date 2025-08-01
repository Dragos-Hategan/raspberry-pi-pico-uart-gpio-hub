/**
 * @file client_side_handshake.c
 * @brief Handles UART handshake from the client side.
 *
 * Implements the logic for scanning UART pin pairs, initiating a handshake
 * with the server, validating echo responses, and confirming connection.
 *
 * - Sends a request of the form "Requesting Connection-[tx,rx]"
 * - Waits for the server to echo "[tx,rx]"
 * - Responds with "[Connection Accepted]" if valid
 * - Stores working connection in global `active_uart_client_connection`
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "functions.h"
#include "config.h"
#include "client.h"

uart_connection_t active_uart_client_connection;

/**
 * @brief Client-side handshake logic: validates server echo and sends ACK.
 *
 * - Waits for server to echo the pin pair.
 * - Compares it to the pin pair originally sent.
 * - Sends "[Connection Accepted]" if the echo is valid.
 *
 * @param uart_instance UART interface used for communication.
 * @param pin_pair The TX/RX pair originally tested.
 * @param timeout_ms Timeout in milliseconds for the response.
 * @return true if the handshake is successful, false otherwise.
 */
static bool client_uart_read(uart_inst_t* uart_instance, uart_pin_pair_t pin_pair, uint32_t timeout_ms){
    char buf[32] = {0};
    uint8_t received_number_pair[2] = {0};

    get_uart_buffer(uart_instance, buf, sizeof(buf), timeout_ms);
    get_number_pair(received_number_pair, buf);

    uint8_t expected_tx_number = received_number_pair[0];
    uint8_t expected_rx_number = received_number_pair[1];

    if (expected_tx_number == pin_pair.tx && expected_rx_number == pin_pair.rx){
        char accepted[strlen(CONNECTION_ACCEPTED_MESSAGE) + 3];
        snprintf(accepted, sizeof(accepted), "[%s]", CONNECTION_ACCEPTED_MESSAGE); 
        uart_puts(uart_instance, accepted);
        uart_tx_wait_blocking(uart_instance);
        return true;
    }

    return false;
}

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
static bool client_test_uart_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance) {
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);

    char message_with_pin_pair[32];
    snprintf(message_with_pin_pair, sizeof(message_with_pin_pair), "%s-[%d,%d]", CONNECTION_REQUEST_MESSAGE, pin_pair.tx, pin_pair.rx);
    uart_puts(uart_instance, message_with_pin_pair);
    uart_tx_wait_blocking(uart_instance);

    return client_uart_read(uart_instance, pin_pair, CLIENT_TIMEOUT_MS);
}

/**
 * @brief Stores the working UART connection in the global state.
 *
 * @param pin_pair The successful TX/RX pin pair.
 * @param uart_instance Pointer to the UART peripheral used.
 */
static inline void client_add_connection(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    active_uart_client_connection.pin_pair = pin_pair;
    active_uart_client_connection.uart_instance = uart_instance;
}

/**
 * @brief Searches UART0 pin pairs for a valid connection with the server.
 *
 * Iterates through all configured UART0 TX/RX combinations, testing each one via `client_test_uart_pair(void)`.
 *
 * @return true if a valid UART0 connection is found, false otherwise.
 */
static bool client_find_connection_for_uart0_instance(void){
    for (uint8_t index = 0; index < PIN_PAIRS_UART0_LEN; index++){
        if(client_test_uart_pair(pin_pairs_uart0[index], uart0)){
            client_add_connection(pin_pairs_uart0[index], uart0);
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
 * Iterates through all configured UART1 TX/RX combinations, testing each one via `client_test_uart_pair(void)`.
 *
 * @return true if a valid UART1 connection is found, false otherwise.
 */
static bool client_find_connection_for_uart1_instance(void){
    for (uint8_t index = 0; index < PIN_PAIRS_UART1_LEN; index++){
        if(client_test_uart_pair(pin_pairs_uart1[index], uart1)){
            client_add_connection(pin_pairs_uart1[index], uart1);
            return true;
        }else{
            reset_gpio_pins(pin_pairs_uart1[index]);
        }
    }
    return false;
}

bool client_detect_uart_connection(void){
    bool connection_found = false;
    connection_found = client_find_connection_for_uart0_instance();
    if (!connection_found){
        connection_found = client_find_connection_for_uart1_instance();
    }
    if (connection_found){
        blink_onboard_led_blocking();
    }

    return connection_found;
}