/**
 * @file client_communication.c
 * @brief Handles UART communication between the server and multiple Pico clients.
 *
 * This module provides helper functions for:
 * - Sending messages over UART safely using spinlocks
 * - Waking up clients from dormant mode
 * - Sending predefined flag messages to specific or all clients
 * - Broadcasting client state information
 * - Coordinating dormant transitions
 *
 * All transmissions ensure UART reinitialization and GPIO reset for consistent operation.
 *
 * @note Functions in this file depend on `active_uart_server_connections` and shared UART locks.
 *
 * @see server.h
 * @see functions.h
 */

#include "hardware/uart.h"

#include "server.h"
#include "functions.h"

void send_uart_message_safe(uart_inst_t* uart, uart_pin_pair_t pins, const char* msg) {
    uint32_t irq = spin_lock_blocking(uart_lock);
    uart_init_with_pins(uart, pins, DEFAULT_BAUDRATE);
    uart_puts(uart, msg);
    uart_tx_wait_blocking(uart);
    reset_gpio_pins(pins);
    spin_unlock(uart_lock, irq);
}

/**
 * @brief Wakes up a client device by toggling RX pin and sending a wake-up message.
 *
 * This function:
 * - Drives the client's RX pin high for 5 ms, then low for 5 ms (to exit dormant mode)
 * - Sends a predefined wake-up flag message over UART to ensure proper synchronization
 *
 * @param pin_pair The TX/RX pin pair used for communication with the client.
 * @param uart     UART instance used to send the message.
 */
static void wake_up_client(uart_pin_pair_t pin_pair, uart_inst_t* uart){
    gpio_put(pin_pair.rx, true);
    sleep_ms(5);
    gpio_put(pin_pair.rx, false);
    sleep_ms(5);

    char msg[8];
    snprintf(msg, sizeof(msg), "[%d,%d]", WAKE_UP_FLAG_NUMBER, WAKE_UP_FLAG_NUMBER);
    send_uart_message_safe(uart, pin_pair, msg);
}

void send_wakeup_if_dormant(uint32_t flash_client_index, server_persistent_state_t *const state, uart_pin_pair_t pin_pair, uart_inst_t *const uart){
    uint8_t active_uart_connection_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, *state);
    if(active_uart_server_connections[active_uart_connection_index].is_dormant){
        wake_up_client(pin_pair, uart);
    }
}

void send_dormant_flag_to_client(uint8_t client_index){
    char msg[8];
    snprintf(msg, sizeof(msg), "[%d,%d]", DORMANT_FLAG_NUMBER, DORMANT_FLAG_NUMBER);
    send_uart_message_safe(active_uart_server_connections[client_index].uart_instance,
        active_uart_server_connections[client_index].pin_pair,
        msg);
}

/**
 * @brief Sends a dormant flag message if the client is marked as dormant.
 *
 * Checks the `is_dormant` flag for the client at the given index, and if true,
 * calls the function to send a dormant message via UART.
 *
 * @param client_index Index of the client in the active server connections.
 */
static void send_dormant_if_is_dormant_is_true(uint8_t client_index){
    if(active_uart_server_connections[client_index].is_dormant){
        send_dormant_flag_to_client(client_index);
    }
}

/**
 * @brief Sends a predefined flag message to a specific client via UART.
 *
 * Constructs a message of the form "[X,X]" using the given flag value
 * and sends it to the specified client over its associated UART instance.
 *
 * @param FLAG_MESSAGE The numeric flag to send (e.g., blink, reset, etc.).
 * @param client_index Index of the client in the active connection list.
 */
static void send_flag_message_to_client(const uint8_t FLAG_MESSAGE, uint8_t client_index){
    char msg[8];
    snprintf(msg, sizeof(msg), "[%d,%d]", FLAG_MESSAGE, FLAG_MESSAGE);
    send_uart_message_safe(active_uart_server_connections[client_index].uart_instance,
        active_uart_server_connections[client_index].pin_pair,
        msg);

    send_dormant_if_is_dormant_is_true(client_index);
}

/**
 * @brief Sends a predefined flag message to all connected clients via UART.
 *
 * Iterates through all active UART client connections. Each client is first
 * woken up, then receives a flag message of the form "[X,X]", where X is the
 * specified flag value.
 *
 * @param FLAG_MESSAGE The numeric flag to send to each client.
 */
static void send_flag_message_to_all_clients(const uint8_t FLAG_MESSAGE){
    for (uint8_t client_index = 0; client_index < active_server_connections_number; client_index++){
        if (active_uart_server_connections[client_index].is_dormant){
            wake_up_client(active_uart_server_connections[client_index].pin_pair,
                active_uart_server_connections[client_index].uart_instance);
        }
        send_flag_message_to_client(FLAG_MESSAGE, client_index);
    } 
}

void signal_reset_for_all_clients(){
    send_flag_message_to_all_clients(TRIGGER_RESET_FLAG_NUMBER);
}

void send_fast_blink_onboard_led_to_clients(){
    send_flag_message_to_all_clients(BLINK_ONBOARD_LED_FLAG_NUMBER);
}

void send_dormant_to_standby_clients(void){
    for (uint8_t active_connection_index = 0; active_connection_index < active_server_connections_number; active_connection_index++){
        send_dormant_if_is_dormant_is_true(active_connection_index);
    }
}

void server_send_client_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, const client_state_t* state){
    wake_up_client(pin_pair, uart);
    uint32_t irq = spin_lock_blocking(uart_lock);
    uart_init_with_pins(uart, pin_pair, DEFAULT_BAUDRATE);

    for (uint8_t i = 0; i < MAX_NUMBER_OF_GPIOS; i++) {
        char msg[8];
        snprintf(msg, sizeof(msg), "[%d,%d]", state->devices[i].gpio_number, state->devices[i].is_on);
        uart_puts(uart, msg);
        uart_tx_wait_blocking(uart);
        sleep_us(500);
    }

    reset_gpio_pins(pin_pair);
    spin_unlock(uart_lock, irq);
}