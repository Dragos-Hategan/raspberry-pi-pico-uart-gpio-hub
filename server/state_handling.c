/**
 * @file state_handling.c
 * @brief Persistent client state handling (save/load/modify) for UART server.
 *
 * This module manages:
 * - Saving the server-side state (all clients and their configs) to flash
 * - Loading and validating state using CRC32
 * - Updating device states on clients and flash
 * - Sending configurations to clients via UART
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "hardware/sync.h"

#include "types.h"
#include "server.h"
#include "functions.h"
#include "input.h"

/**
 * @brief Computes CRC32 checksum over a block of memory.
 *
 * @param data Pointer to the data block.
 * @param length Number of bytes to process.
 * @return uint32_t CRC32 checksum.
 */
uint32_t compute_crc32(const void *data, uint32_t length) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;

    for (uint32_t i = 0; i < length; ++i) {
        crc ^= bytes[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return ~crc;
}

void __not_in_flash_func(save_server_state)(const server_persistent_state_t *state_in) {
    server_persistent_state_t temp;
    memcpy(&temp, state_in, sizeof(temp));

    temp.crc = 0;
    temp.crc = compute_crc32(&temp, sizeof(temp));

    uint8_t buffer[SERVER_SECTOR_SIZE] = {0};
    memcpy(buffer, &temp, sizeof(temp));

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(SERVER_FLASH_OFFSET, SERVER_SECTOR_SIZE);
    flash_range_program(SERVER_FLASH_OFFSET, buffer, SERVER_SECTOR_SIZE);
    restore_interrupts(ints);
}

bool load_server_state(server_persistent_state_t *out_state) {
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    memcpy(out_state, flash_state, sizeof(server_persistent_state_t));

    uint32_t saved_crc = out_state->crc;
    out_state->crc = 0;
    uint32_t computed_crc = compute_crc32(out_state, sizeof(server_persistent_state_t));
    out_state->crc = saved_crc;

    return (saved_crc == computed_crc);
}

/**
 * @brief Sends a single GPIO device state to a client via UART.
 *
 * @param pin_pair UART TX/RX pin pair to use.
 * @param uart UART instance.
 * @param gpio_number GPIO number on the client to modify.
 * @param is_on true = turn on, false = turn off.
 */
static void server_send_device_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, uint8_t gpio_number, bool is_on){
    uart_init_with_pins(uart, pin_pair, DEFAULT_BAUDRATE);
    char msg[8];
    snprintf(msg, sizeof(msg), "[%d,%d]", gpio_number, is_on);
    uart_puts(uart, msg);
    sleep_ms(10);
    reset_gpio_pins(pin_pair);
}

void server_send_client_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, const client_state_t* state){
    uart_init_with_pins(uart, pin_pair, DEFAULT_BAUDRATE);
    for (uint8_t i = 0; i < MAX_NUMBER_OF_GPIOS; i++) {
        char msg[8];
        snprintf(msg, sizeof(msg), "[%d,%d]", state->devices[i].gpio_number, state->devices[i].is_on);
        uart_puts(uart, msg);
        sleep_ms(10);
    }
    reset_gpio_pins(pin_pair);
}

/**
 * @brief Loads the running state for an active client and sends it over UART.
 *
 * - Marks the client as active
 * - Sends the device states to the client
 *
 * @param server_uart_connection Connection info (pin pair + instance).
 * @param server_persistent_state Pointer to loaded flash state.
 */
static void server_load_client_state(server_uart_connection_t server_uart_connection, server_persistent_state_t *server_persistent_state) {
    for (uint8_t i = 0; i < MAX_SERVER_CONNECTIONS; i++) {
        client_t *saved_client = &server_persistent_state->clients[i];

        if (saved_client->uart_connection.pin_pair.tx == server_uart_connection.pin_pair.tx &&
            saved_client->uart_connection.pin_pair.rx == server_uart_connection.pin_pair.rx &&
            saved_client->uart_connection.uart_instance == server_uart_connection.uart_instance) {
            
            saved_client->is_active = true;
            server_send_client_state(server_uart_connection.pin_pair, server_uart_connection.uart_instance, &saved_client->running_client_state);
            return;
        }
    }
}

/**
 * @brief Marks UART pins as reserved in the running config of a client.
 *
 * - Used to prevent GPIO conflicts for UART communication pins.
 *
 * @param client_list_index Index of the client in flash.
 * @param server_persistent_state Pointer to full persistent state.
 */
static void configure_running_state_uart_connection_pins(uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t index = 0; index < active_server_connections_number; index++) {
        client_t *client = &server_persistent_state->clients[client_list_index];
        if (active_uart_server_connections[index].pin_pair.tx == client->uart_connection.pin_pair.tx){
            client->running_client_state.devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.tx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
            client->running_client_state.devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.rx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
            client->is_active = true;
        }
    }
}

void server_reset_configuration(client_state_t *client_state){
    for (uint8_t index = 0; index < MAX_NUMBER_OF_GPIOS; index++){
        if (client_state->devices[index].gpio_number != UART_CONNECTION_FLAG_NUMBER){
            client_state->devices[index].is_on = false;
        }
    }
}

/**
 * @brief Initializes the current (live) GPIO states for a client.
 *
 * - Assigns GPIO numbers and sets all to off.
 * - Flags UART pins to avoid conflict.
 *
 * @param client_list_index Index of the client.
 * @param server_persistent_state Persistent state structure.
 */
static void configure_running_state(uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t index = 0; index < MAX_NUMBER_OF_GPIOS; index++){
        server_persistent_state->clients[client_list_index].running_client_state.devices[index].gpio_number = index + ((index / 23) * 3);
        server_persistent_state->clients[client_list_index].running_client_state.devices[index].is_on = false;
    }

    configure_running_state_uart_connection_pins(client_list_index, server_persistent_state);
}

void set_configuration_devices(uint32_t flash_client_index, uint32_t flash_configuration_index){
    server_persistent_state_t state;
    load_server_state(&state);

    while(true){
        input_client_data_t input_client_data = {0};
        client_input_flags_t client_input_flags = {0};
        client_input_flags.need_device_index = true;
        client_input_flags.need_device_state = true;

        if (read_client_data(&input_client_data, client_input_flags)){
            state.clients[flash_client_index].preset_configs[flash_configuration_index].devices[input_client_data.device_index - 1].is_on = input_client_data.device_state;
            save_server_state(&state);
        }else{
            return;
        }
    }
}

/**
 * @brief Marks UART pins as reserved in the preset configurations.
 *
 * @param client_list_index Index of the client.
 * @param server_persistent_state Persistent state pointer.
 * @param config_index Preset configuration index to modify.
 */
static void configure_preset_configs_uart_connection_pins(uint8_t client_list_index, server_persistent_state_t *server_persistent_state, uint8_t config_index){
    for (uint8_t index = 0; index < active_server_connections_number; index++) {
        client_t *client = &server_persistent_state->clients[client_list_index];
        if (active_uart_server_connections[index].pin_pair.tx == client->uart_connection.pin_pair.tx){
            client->preset_configs[config_index].devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.tx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
            client->preset_configs[config_index].devices[active_uart_server_connections[index].uart_pin_pair_from_client_to_server.rx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
        }
    }
}

/**
 * @brief Initializes all preset configurations for a client.
 *
 * - Fills in GPIO numbers and disables all.
 * - Marks UART pins as reserved.
 *
 * @param client_list_index Client index.
 * @param server_persistent_state Persistent state struct.
 */
static void configure_preset_configs(uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t config_index = 0; config_index < NUMBER_OF_POSSIBLE_PRESETS; config_index++){
        for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
            server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[gpio_index].gpio_number = gpio_index + ((gpio_index / 23) * 3);
            server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[gpio_index].is_on = false;
        }

        configure_preset_configs_uart_connection_pins(client_list_index, server_persistent_state, config_index);
    }
}

/**
 * @brief Initializes a client entry in the persistent state.
 *
 * - Saves UART pin pair and instance.
 * - Sets is_active to false.
 * - Calls config functions.
 *
 * @param uart_pin_pair TX/RX pin pair for the client.
 * @param client_list_index Index in client array.
 * @param server_persistent_state State structure to populate.
 * @param uart_inst UART instance.
 */
static void configure_client(uart_pin_pair_t uart_pin_pair, uint8_t client_list_index, server_persistent_state_t *server_persistent_state, uart_inst_t* uart_inst){
    server_persistent_state->clients[client_list_index].uart_connection.pin_pair = uart_pin_pair;
    server_persistent_state->clients[client_list_index].uart_connection.uart_instance = uart_inst;
    server_persistent_state->clients[client_list_index].is_active = false;

    configure_running_state(client_list_index, server_persistent_state);
    configure_preset_configs(client_list_index, server_persistent_state);
}

/**
 * @brief Fills the entire persistent state structure and saves it to flash.
 *
 * - Called when flash is empty or invalid.
 * - Configures all known clients.
 *
 * @param server_persistent_state Pointer to state struct to fill and write.
 */
void server_configure_persistent_state(server_persistent_state_t *server_persistent_state) {
    uint8_t client_list_index = 0;
    for (uint8_t i = 0; i < PIN_PAIRS_UART0_LEN; i++) {
        configure_client(pin_pairs_uart0[i], client_list_index, server_persistent_state, uart0);
        client_list_index++;
    }
    for (uint8_t i = 0; i < PIN_PAIRS_UART1_LEN; i++) {
        configure_client(pin_pairs_uart1[i], client_list_index, server_persistent_state, uart1);
        client_list_index++;
    }
    save_server_state(server_persistent_state);
}

void server_print_state_devices(const client_state_t *client_state){
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, client_state);
    }
}

/**
 * @brief Prints the live (running) GPIO state for a single client.
 *
 * @param client Pointer to the client structure.
 */
void server_print_running_client_state(const client_t *client){
    printf("Running Client State Devices:\n");
    server_print_state_devices(&client->running_client_state);
}

/**
 * @brief Prints a specified preset configuration for a client.
 *
 * @param client Pointer to the client.
 * @param client_preset_index Preset index to print.
 */
void server_print_client_preset_configuration(const client_t *client, uint8_t client_preset_index){
    printf("Preset Config[%u] Devices:\n", client_preset_index + 1);
    server_print_state_devices(&client->preset_configs[client_preset_index]);
}

void server_print_client_preset_configurations(const client_t *client){
    for (uint32_t preset_config_index = 0; preset_config_index < NUMBER_OF_POSSIBLE_PRESETS; preset_config_index++){
        server_print_client_preset_configuration(client, preset_config_index);
        printf("\n");
    }
}

/**
 * @brief Prints the full contents of the server's persistent state from flash.
 *
 * - Iterates through all known clients (connected or not).
 * - For each client:
 *    - Prints whether it's active.
 *    - Prints the current (running) GPIO state.
 *    - Prints all saved preset configurations.
 *    - Shows UART connection info.
 * - Also prints the CRC and total size of the structure.
 *
 * Useful for debugging persistent storage, flash layout, and saved configurations.
 *
 * @param server_persistent_state Pointer to the server's full persistent state structure.
 */
static void server_print_persistent_state(server_persistent_state_t *server_persistent_state){
    for (uint8_t server_connections_index = 0; server_connections_index < MAX_SERVER_CONNECTIONS; server_connections_index++){
        const client_t *client = &server_persistent_state->clients[server_connections_index];
        printf("Client[%u]:\n", server_connections_index + 1);
        printf("Is active = %s\n", client->is_active ? "YES" : "NO");

        server_print_running_client_state(client);
        server_print_client_preset_configurations(client);

        printf("\nUart connection:\nPin TX: %u, Pin RX: %u, Uart Instance Number: %u\n",
            client->uart_connection.pin_pair.tx,
            client->uart_connection.pin_pair.rx,
            UART_NUM(client->uart_connection.uart_instance)
        );

        printf("\n\n");
    }

    printf("CRC: %u\n", server_persistent_state->crc);

    printf("\n\n");

    _Static_assert(sizeof(server_persistent_state_t) <= 4096, "Struct too large for one flash sector!");
    printf("sizeof(server_persistent_state_t): %u\n", sizeof(server_persistent_state_t));   
    printf("sizeOf server_persistent_state_t: %d", sizeof(server_persistent_state_t));
    printf("\n\n\n\n");
}

/**
 * @brief Loads and applies all saved client states to active clients.
 *
 * - Verifies CRC from flash.
 * - Sends state to each active client.
 * - Re-initializes flash if data is corrupted.
 */
void server_load_running_states_to_active_clients(void) {
    server_persistent_state_t server_persistent_state = {0};
    bool valid_crc = load_server_state(&server_persistent_state);

    if (valid_crc) {
        printf("LOADING ATTEMPT SUCCESSFULL!\nLoading states.\n");
        for (uint8_t index = 0; index < active_server_connections_number; index++) {
            server_load_client_state(active_uart_server_connections[index], &server_persistent_state);
        }
    } else {
        printf("LOADING ATTEMPT FAILED!\nFirst run or flash problem.\nInitializing Configuration\n");
        server_configure_persistent_state(&server_persistent_state);
        printf("Configuration was successfull\n");

    }

    //server_print_persistent_state(&server_persistent_state);
}

void server_set_device_state_and_update_flash(uart_pin_pair_t pin_pair, uart_inst_t* uart_instance, uint8_t gpio_index, bool device_state, uint32_t flash_client_index){
    server_send_device_state(pin_pair, uart_instance, gpio_index, device_state);
    
    server_persistent_state_t state_copy;
    memcpy(&state_copy, (const server_persistent_state_t *)SERVER_FLASH_ADDR, sizeof(state_copy));
    
    state_copy.clients[flash_client_index].running_client_state.devices[gpio_index > 22 ? (gpio_index - 3) : (gpio_index)].is_on = device_state;
    
    save_server_state(&state_copy);
}
