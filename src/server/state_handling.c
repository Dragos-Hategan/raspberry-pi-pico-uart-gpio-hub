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
#include "pico/multicore.h"

#include "types.h"
#include "server.h"
#include "functions.h"
#include "input.h"

/**
 * @brief Sends a UART message safely using spinlock protection.
 *
 * This function first wakes up the client by sending a pulse on its RX pin.
 * Then it initializes the UART peripheral with the specified TX/RX pins and baudrate,
 * sends the message, waits for the transmission to complete, and resets the GPIO pins.
 * All UART operations are protected by a spinlock to ensure thread/core safety.
 *
 * @param uart Pointer to the UART instance to use (e.g., uart0 or uart1).
 * @param pins Struct containing the TX and RX GPIO pin numbers.
 * @param msg Null-terminated string to be sent via UART.
 */
static void send_uart_message_safe(uart_inst_t* uart, uart_pin_pair_t pins, const char* msg) {
    uint32_t irq = spin_lock_blocking(uart_lock);
    uart_init_with_pins(uart, pins, DEFAULT_BAUDRATE);
    uart_puts(uart, msg);
    uart_tx_wait_blocking(uart);
    reset_gpio_pins(pins);
    //uart_deinit(uart);
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

/**
 * @brief Retrieves the index of an active client connection matching a flash-stored client.
 *
 * Iterates through active UART server connections and compares their TX pins
 * with the TX pin of the client at the specified index in the persistent flash state.
 *
 * @param flash_client_index Index of the client in the persistent flash state array.
 * @param state The loaded persistent server state.
 * @return Index of the matching active connection, or INVALID_CLIENT_INDEX if not found.
 */
static uint32_t get_active_client_connection_index_from_flash_client_index(uint32_t flash_client_index, server_persistent_state_t state){
    for (uint8_t active_client_index = 0; active_client_index < active_server_connections_number; active_client_index++){
        if (active_uart_server_connections[active_client_index].pin_pair.tx == state.clients[flash_client_index].uart_connection.pin_pair.tx){
            return active_client_index;
        }
    }
    return INVALID_CLIENT_INDEX;
}

/**
 * @brief Wakes up a dormant client if needed, based on its persistent state.
 *
 * Checks whether the client at the specified index is dormant. If so, it
 * sends a wake-up pulse via UART to ensure the client is active before communication.
 *
 * @param flash_client_index Index of the client in the persistent state table.
 * @param state              Pointer to the global persistent state structure.
 * @param pin_pair           UART TX/RX pins used to wake up the client.
 * @param uart               Pointer to the UART instance used for transmission.
 *
 * @see wake_up_client()
 */
static void send_wakeup_if_dormant(uint32_t flash_client_index, server_persistent_state_t *const state, uart_pin_pair_t pin_pair, uart_inst_t *const uart){
    uint8_t active_uart_connection_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, *state);
    if(active_uart_server_connections[active_uart_connection_index].is_dormant){
        wake_up_client(pin_pair, uart);
    }
}

/**
 * @brief Prints the GPIO state of a device at the given index in a client.
 *
 * This is an inline helper function. If the GPIO is used for UART, it is
 * marked as restricted.
 *
 * @param gpio_index Index of the device in the client_state array.
 * @param client_state Pointer to the client_state_t structure.
 */
static void server_print_gpio_state(uint8_t gpio_index, const client_state_t *client_state){
    if (client_state->devices[gpio_index].gpio_number == UART_CONNECTION_FLAG_NUMBER){
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "%2u. UART connection, no access.\n", gpio_index + 1);
        printf_and_update_buffer(string);
    }else{
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "%2u. GPIO_NO: %2u  Power: %s\n",
            gpio_index + 1,
            client_state->devices[gpio_index].gpio_number,
            client_state->devices[gpio_index].is_on ? "ON" : "OFF");
        printf_and_update_buffer(string);
    }
}

void server_print_state_devices(const client_state_t *client_state){
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, client_state);
    }
}

void server_print_running_client_state(const client_t *client){
    printf_and_update_buffer("Running Client State Devices:\n");
    server_print_state_devices(&client->running_client_state);
}

void server_print_client_preset_configuration(const client_t *client, uint8_t client_preset_index){
    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "Preset Config[%u] Devices:\n", client_preset_index + 1);
    printf_and_update_buffer(string);
    server_print_state_devices(&client->preset_configs[client_preset_index]);
}

void server_print_client_preset_configurations(const client_t *client){
    for (uint32_t preset_config_index = 0; preset_config_index < NUMBER_OF_POSSIBLE_PRESETS; preset_config_index++){
        server_print_client_preset_configuration(client, preset_config_index);
        printf_and_update_buffer("\n");
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

/**
 * @brief Sends a dormant message to all clients marked as dormant.
 *
 * Iterates through all active UART connections and sends a predefined
 * dormant flag message to each client with the `is_dormant` flag set.
 */
static void send_dormant_to_standby_clients(void){
    for (uint8_t active_connection_index = 0; active_connection_index < active_server_connections_number; active_connection_index++){
        send_dormant_if_is_dormant_is_true(active_connection_index);
    }
}

bool client_has_active_devices(client_t client){
    for (uint8_t device_index = 0; device_index < MAX_NUMBER_OF_GPIOS; device_index++){
        if (client.running_client_state.devices[device_index].is_on){
            return true;
        }
    }
    return false;
}

/**
 * @brief Updates the dormant status of all connected clients based on their active devices.
 *
 * For each active UART client, this function compares its TX pin with the
 * persistent state list and sets its `is_dormant` flag to true if all devices are OFF.
 *
 * @param server_persistent_state Pointer to the saved state containing all client info.
 */
static void set_dormant_flag_to_standby_clients(server_persistent_state_t *server_persistent_state){
    for (uint8_t active_client_index = 0; active_client_index < active_server_connections_number; active_client_index++){
        for (uint8_t persistent_state_client_index = 0; persistent_state_client_index < MAX_SERVER_CONNECTIONS; persistent_state_client_index++){
            if (active_uart_server_connections[active_client_index].pin_pair.tx == server_persistent_state->clients[persistent_state_client_index].uart_connection.pin_pair.tx){
                active_uart_server_connections[active_client_index].is_dormant = !client_has_active_devices(server_persistent_state->clients[persistent_state_client_index]);
            }
        }
    }
}

/**
 * @brief Computes CRC32 checksum over a block of memory.
 *
 * @param data Pointer to the data block.
 * @param length Number of bytes to process.
 * @return uint32_t CRC32 checksum.
 */
static uint32_t compute_crc32(const void *data, uint32_t length) {
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

/**
 * @brief Loads the server state from flash and validates it using CRC32.
 *
 * @param out_state Pointer to destination structure to store loaded state.
 * @return true if CRC is valid and data is intact, false otherwise.
 */
static bool load_server_state(server_persistent_state_t *out_state) {
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    memcpy(out_state, flash_state, sizeof(server_persistent_state_t));

    uint32_t saved_crc = out_state->crc;
    out_state->crc = 0;
    uint32_t computed_crc = compute_crc32(out_state, sizeof(server_persistent_state_t));
    out_state->crc = saved_crc;

    return (saved_crc == computed_crc);
}

/**
 * @brief Saves the persistent server state structure to flash memory.
 *
 * - Computes CRC
 * - Copies the data to a buffer
 * - Erases and programs the flash sector
 *
 * @param state_in Pointer to the server_persistent_state_t structure to save.
 */
static void __not_in_flash_func(save_server_state)(const server_persistent_state_t *state_in) {
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

void set_configuration_devices(uint32_t flash_client_index, uint32_t flash_configuration_index, input_client_data_t *input_client_data){
    server_persistent_state_t state;
    load_server_state(&state);

    while(true){
        uint32_t device_index;
        read_device_index(&device_index,
            flash_client_index,
            &state,
            &state.clients[flash_client_index].preset_configs[flash_configuration_index]
        );
        if (!device_index){
            return;
        }
    
        uint32_t device_state;
        read_device_state(&device_state);
        if (!device_state){
            return;
        }
        device_state %= 2;

        state.clients[flash_client_index].preset_configs[flash_configuration_index].devices[device_index - 1].is_on = device_state;

        save_server_state(&state);
    }
}

/**
 * @brief Resets the runtime GPIO configuration for a given client.
 *
 * This function iterates through all devices associated with the client
 * and sets their `is_on` flag to false, effectively turning them off.
 * Devices marked with the special UART connection flag are excluded from this reset.
 *
 * @param[in,out] client_state Pointer to the client's current runtime state to be modified.
 */
static void server_reset_configuration(client_state_t *client_state){
    for (uint8_t index = 0; index < MAX_NUMBER_OF_GPIOS; index++){
        if (client_state->devices[index].gpio_number != UART_CONNECTION_FLAG_NUMBER){
            client_state->devices[index].is_on = false;
        }
    }
}

/**
 * @brief Sends the entire current client state over UART.
 *
 * @param pin_pair UART TX/RX pin pair to use.
 * @param uart UART instance.
 * @param state Pointer to the client_state_t to send.
 */
static void server_send_client_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, const client_state_t* state){
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

void reset_running_configuration(uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    server_reset_configuration(&state.clients[flash_client_index].running_client_state);

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);

    uint8_t active_client_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, state);
    send_dormant_flag_to_client(active_client_index);
    active_uart_server_connections[active_client_index].is_dormant = true;
    
    save_server_state(&state);

    printf_and_update_buffer("\nRunning Configuration Reset.\n");
}

void reset_preset_configuration(uint32_t flash_client_index, uint32_t flash_configuration_index){
    server_persistent_state_t state;
    load_server_state(&state);
    server_reset_configuration(&state.clients[flash_client_index].preset_configs[flash_configuration_index - 1]);

    save_server_state(&state);

    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "\nPreset Configuration [%u] Reset.\n", flash_configuration_index);
    printf_and_update_buffer(string);
}

void reset_all_client_data(uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);
    server_reset_configuration(&state.clients[flash_client_index].running_client_state);

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);

    uint8_t active_client_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, state);
    send_dormant_flag_to_client(active_client_index);
    active_uart_server_connections[active_client_index].is_dormant = true;

    for (uint8_t configuration_index = 0; configuration_index < NUMBER_OF_POSSIBLE_PRESETS; configuration_index++){
        server_reset_configuration(&state.clients[flash_client_index].preset_configs[configuration_index]);
    }

    save_server_state(&state);
    printf_and_update_buffer("\nAll Client Data Reset.\n");
}

void load_configuration_into_running_state(uint32_t flash_configuration_index, uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    memcpy(
        &state.clients[flash_client_index].running_client_state,
        &state.clients[flash_client_index].preset_configs[flash_configuration_index],
        sizeof(client_state_t));

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);
    save_server_state(&state);

    
    uint8_t active_client_index = get_active_client_connection_index_from_flash_client_index(flash_client_index, state);
    if (!client_has_active_devices(state.clients[flash_client_index])){
        send_dormant_flag_to_client(active_client_index);
        active_uart_server_connections[active_client_index].is_dormant = true;
    }else{
        active_uart_server_connections[active_client_index].is_dormant = false;
    }

    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "\nConfiguration Preset[%u] Loaded!\n", flash_configuration_index + 1);
    printf_and_update_buffer(string);
}

void save_running_configuration_into_preset_configuration(uint32_t flash_configuration_index, uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    memcpy(
        &state.clients[flash_client_index].preset_configs[flash_configuration_index],
        &state.clients[flash_client_index].running_client_state,
        sizeof(client_state_t));
    
    save_server_state(&state);

    char string[BUFFER_MAX_STRING_SIZE];
    snprintf(string, sizeof(string), "\nConfiguration saved in Preset[%u].\n", flash_configuration_index + 1);
    printf_and_update_buffer(string);
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

/**
 * @brief Initializes a client entry in the persistent state.
 *
 * - Saves UART pin pair and instance.
 * - Sets is_dormant to true.
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
static void server_configure_persistent_state(server_persistent_state_t *server_persistent_state) {
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
    for (uint8_t flash_client_index = 0; flash_client_index < MAX_SERVER_CONNECTIONS; flash_client_index++) {
        client_t *saved_client = &server_persistent_state->clients[flash_client_index];
        
        if (saved_client->uart_connection.pin_pair.tx == server_uart_connection.pin_pair.tx &&
            saved_client->uart_connection.pin_pair.rx == server_uart_connection.pin_pair.rx &&
            saved_client->uart_connection.uart_instance == server_uart_connection.uart_instance) {
                
            server_send_client_state(server_uart_connection.pin_pair, server_uart_connection.uart_instance, &saved_client->running_client_state);
            return;
        }
    }
}

void server_load_running_states_to_active_clients(void){
    server_persistent_state_t server_persistent_state = {0};
    bool valid_crc = load_server_state(&server_persistent_state);

    if (valid_crc) {
        printf("LOADING ATTEMPT SUCCESSFULL!\nLoading states.\n");
        for (uint8_t index = 0; index < active_server_connections_number; index++) {
            server_load_client_state(active_uart_server_connections[index], &server_persistent_state);
        }
    } else {
        printf("LOADING ATTEMPT FAILED!\nIncorrect CRC, this is the first run after build or might be a flash problem.\nInitializing Configuration...\n");
        server_configure_persistent_state(&server_persistent_state);
        printf("CONFIGURATION WAS SUCCESSFULL!\nStarting...\n");
    }

    set_dormant_flag_to_standby_clients(&server_persistent_state);
    send_dormant_to_standby_clients();
}

/**
 * @brief Sends the current state of a GPIO to a client device via UART.
 *
 * Wakes up the specified client using a TX pulse and then sends a message
 * representing the GPIO number and its current state (ON or OFF) in the format "[N,S]",
 * where `N` is the GPIO number and `S` is 1 (ON) or 0 (OFF).
 *
 * @param pin_pair            UART TX/RX pin pair used for the target client.
 * @param uart                Pointer to the UART instance used for transmission.
 * @param gpio_number         The GPIO number whose state is being sent.
 * @param is_on               The current state of the GPIO (`true` for ON, `false` for OFF).
 * @param state               Pointer to the global server persistent state (used to access client status).
 * @param flash_client_index  Index of the client in the persistent state table.
 *
 * @note This function uses `send_wakeup_if_dormant()` before sending the state,
 *       ensuring the client is awake before communication.
 *
 * @see send_wakeup_if_dormant()
 * @see send_uart_message_safe()
 */
static void server_send_device_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, uint8_t gpio_number, bool is_on, server_persistent_state_t *const state, uint32_t flash_client_index){
    send_wakeup_if_dormant(flash_client_index, state, pin_pair, uart);

    char msg[8];
    snprintf(msg, sizeof(msg), "[%d,%d]", gpio_number, is_on);
    send_uart_message_safe(uart, pin_pair, msg);
}

void server_set_device_state_and_update_flash(uart_pin_pair_t pin_pair, uart_inst_t* uart_instance, uint8_t gpio_index, bool device_state, uint32_t flash_client_index){
    server_persistent_state_t state_copy;
    memcpy(&state_copy, (const server_persistent_state_t *)SERVER_FLASH_ADDR, sizeof(state_copy));

    server_send_device_state(pin_pair, uart_instance, gpio_index, device_state, &state_copy, flash_client_index);
    state_copy.clients[flash_client_index].running_client_state.devices[gpio_index > 22 ? (gpio_index - 3) : (gpio_index)].is_on = device_state;
    save_server_state(&state_copy);
}
