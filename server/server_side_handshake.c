#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "functions.h"
#include "config.h"
#include "server.h"

server_uart_connection_t active_uart_server_connections[MAX_SERVER_CONNECTIONS];
uint8_t active_server_connections_number = 0;
uart_pin_pair_t actual_client_to_server_pin_pair;

/**
 * @brief Server-side handshake logic: responds to connection requests and validates client ACK.
 *
 * - Reads a request of the form "Requesting Connection-[tx,rx]".
 * - Sends back an echo of the pin pair in the format "[tx,rx]".
 * - Waits for and validates the client's ACK: "[Connection Accepted]".
 *
 * @param uart_instance UART interface used for communication.
 * @param timeout_ms Timeout in milliseconds for each stage.
 * @return true if a complete and valid handshake occurs, false otherwise.
 */
bool server_uart_read(uart_inst_t* uart_instance, uint32_t timeout_ms){
    char buf[32] = {0};
    uint8_t received_number_pair[2] = {0};

    sleep_ms(10);
    get_uart_buffer(uart_instance, buf, sizeof(buf), timeout_ms);

    uint8_t received_tx_number;
    uint8_t received_rx_number;
    if (strlen(buf) > 1){
        get_number_pair(received_number_pair, buf);
        received_tx_number = received_number_pair[0];
        received_rx_number = received_number_pair[1];

        char received_pair[8];
        snprintf(received_pair, sizeof(received_pair), "[%d,%d]", received_tx_number, received_rx_number);
        uart_puts(uart_instance, received_pair);
    }
    else{
        return false;
    }

    char ack_buf[32] = {0};
    get_uart_buffer(uart_instance, ack_buf, sizeof(ack_buf), timeout_ms);

    if (strcmp(ack_buf, "[" CONNECTION_ACCEPTED_MESSAGE "]") == 0){
        actual_client_to_server_pin_pair.tx = received_tx_number;
        actual_client_to_server_pin_pair.rx = received_rx_number;
        return true;
    }

    return false;
}

/**
 * @brief Initializes UART on the specified pin pair and attempts to detect a connection request.
 *
 * This function is used to check whether a client is attempting to connect on the given TX/RX pin pair.
 *
 * @param pin_pair The TX/RX pin pair to test.
 * @param uart_instance Pointer to the UART peripheral (e.g., uart0 or uart1).
 * @return true if a connection request is successfully detected, false otherwise.
 */
static inline bool server_check_pin_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    return server_uart_read(uart_instance, SERVER_TIMEOUT_MS);
}

/**
 * @brief Adds a valid UART connection (pin pair + UART instance) to the active connections list.
 *
 * Only adds the connection if there is room in the `active_uart_server_connections` array.
 *
 * @param pin_pair The TX/RX pin pair that was successfully connected.
 * @param uart_instance Pointer to the UART peripheral associated with the connection.
 */
static inline void server_add_active_pair(uart_pin_pair_t pin_pair, uart_inst_t * uart_instance){
    if (active_server_connections_number < MAX_SERVER_CONNECTIONS) {
        active_uart_server_connections[active_server_connections_number].pin_pair = pin_pair;
        active_uart_server_connections[active_server_connections_number].uart_instance = uart_instance;
        active_uart_server_connections[active_server_connections_number].uart_pin_pair_from_client_to_server.tx = actual_client_to_server_pin_pair.tx;
        active_uart_server_connections[active_server_connections_number].uart_pin_pair_from_client_to_server.rx = actual_client_to_server_pin_pair.rx;
        active_server_connections_number++;
    }
}

/**
 * @brief Scans all UART0 pin pairs for active connection requests.
 *
 * For each TX/RX pair configured for UART0, this function attempts to perform a handshake.
 * If the handshake is successful, the connection is stored.
 */
static void server_check_connections_for_uart0_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART0_LEN; index++){
        if(server_check_pin_pair(pin_pairs_uart0[index], uart0)){
            server_add_active_pair(pin_pairs_uart0[index], uart0);
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
static void server_check_connections_for_uart1_instance(){
    for (uint8_t index = 0; index < PIN_PAIRS_UART1_LEN; index++){
        if(server_check_pin_pair(pin_pairs_uart1[index], uart1)){
            server_add_active_pair(pin_pairs_uart1[index], uart1);
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
bool server_find_connections(){
    server_check_connections_for_uart0_instance();
    server_check_connections_for_uart1_instance();

    if (active_server_connections_number) return true;

    return false;
}
