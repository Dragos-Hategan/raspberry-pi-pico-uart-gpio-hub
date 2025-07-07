#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "types.h"
#include "server.h"
#include "hardware/sync.h"
#include "functions.h"

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

    printf("saved_crc=%lu, computed_crc=%ld\n", saved_crc, computed_crc);

    return (saved_crc == computed_crc);
}

static void configure_running_state(uart_pin_pair_t uart_pin_pair, uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t index = 0; index < MAX_NUMBER_OF_GPIOS; index++){
        server_persistent_state->clients[client_list_index].running_client_state.devices[index].gpio_number = index + ((index / 23) * 3);
        server_persistent_state->clients[client_list_index].running_client_state.devices[index].is_on = false;
    }

    server_persistent_state->clients[client_list_index].running_client_state.devices[uart_pin_pair.tx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
    server_persistent_state->clients[client_list_index].running_client_state.devices[uart_pin_pair.rx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
}

static void configure_preset_configs(uart_pin_pair_t uart_pin_pair, uint8_t client_list_index, server_persistent_state_t *server_persistent_state){
    for (uint8_t config_index = 0; config_index < NUMBER_OF_POSSIBLE_PRESETS; config_index++){
        for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
            server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[gpio_index].gpio_number = gpio_index + ((gpio_index / 23) * 3);
            server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[gpio_index].is_on = false;
        }
        server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[uart_pin_pair.tx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
        server_persistent_state->clients[client_list_index].preset_configs[config_index].devices[uart_pin_pair.rx].gpio_number = UART_CONNECTION_FLAG_NUMBER;
    }
}

static void configure_client(uart_pin_pair_t uart_pin_pair, uint8_t client_list_index, server_persistent_state_t *server_persistent_state, uart_inst_t* uart_inst){
    server_persistent_state->clients[client_list_index].uart_connection.pin_pair = uart_pin_pair;
    server_persistent_state->clients[client_list_index].uart_connection.uart_instance = uart_inst;

    configure_running_state(uart_pin_pair, client_list_index, server_persistent_state);
    configure_preset_configs(uart_pin_pair, client_list_index, server_persistent_state);
}

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

static void server_send_client_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, const client_state_t* state){
    uart_init_with_pins(uart, pin_pair, DEFAULT_BAUDRATE);
    for (uint8_t i = 0; i < MAX_NUMBER_OF_GPIOS; i++) {
        char msg[8];
        snprintf(msg, sizeof(msg), "[%d,%d]", state->devices[i].gpio_number, state->devices[i].is_on);
        uart_puts(uart, msg);
        sleep_ms(10);
    }
    reset_gpio_pins(pin_pair);
}

static void server_load_client_state(uart_connection_t uart_connection, server_persistent_state_t *server_persistent_state) {
    for (uint8_t i = 0; i < MAX_SERVER_CONNECTIONS; i++) {
        client_t *saved_client = &server_persistent_state->clients[i];

        if (saved_client->uart_connection.pin_pair.tx == uart_connection.pin_pair.tx &&
            saved_client->uart_connection.pin_pair.rx == uart_connection.pin_pair.rx &&
            saved_client->uart_connection.uart_instance == uart_connection.uart_instance) {

            server_send_client_state(uart_connection.pin_pair, uart_connection.uart_instance, &saved_client->running_client_state);
            return;
        }
    }
}

static inline void server_print_gpio_state(uint8_t gpio_index, client_state_t *client_state){
    if (client_state->devices[gpio_index].gpio_number == UART_CONNECTION_FLAG_NUMBER){
        printf("UART connection. gpio_number:%u\n", client_state->devices[gpio_index].gpio_number);
    }else{
        printf("Device[%u]: gpio_number:%u is_on:%u\n", gpio_index, client_state->devices[gpio_index].gpio_number, client_state->devices[gpio_index].is_on);
    }
}

void server_print_running_client_state(client_t *client){
    printf("\nRunning Client State:\n");
    client_state_t *running_client_state = &client->running_client_state;
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, running_client_state);
    }
}

void server_print_client_preset_configuration(client_t *client, uint8_t client_preset_index){
    printf("\nPreset Config[%u]:\n", index + 1);
    client_state_t *preset_config = &client->preset_configs[client_preset_index];
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, preset_config);
    }
}

static void server_print_persistent_state(server_persistent_state_t *server_persistent_state){
    for (uint8_t server_connections_index = 0; server_connections_index < MAX_SERVER_CONNECTIONS; server_connections_index++){
        client_t *client = &server_persistent_state->clients[server_connections_index];
        printf("Client[%u]:\n", server_connections_index + 1);

        server_print_running_client_state(client);

        for (uint8_t client_preset_index = 0; client_preset_index < NUMBER_OF_POSSIBLE_PRESETS; client_preset_index++){
            server_print_client_preset_configuration(client, client_preset_index);
        }

        printf("\nUart connection:\nPin TX: %u, Pin RX: %u, Uart Instance Number: %u\n", client->uart_connection.pin_pair.tx, client->uart_connection.pin_pair.rx, UART_NUM(client->uart_connection.uart_instance));
        
        printf("\n\n");
    }

    printf("CRC: %u\n", server_persistent_state->crc);

    printf("\n\n");

    _Static_assert(sizeof(server_persistent_state_t) <= 4096, "Struct too large for one flash sector!");
    printf("sizeof(server_persistent_state_t): %u\n", sizeof(server_persistent_state_t));   
    printf("sizeOf server_persistent_state_t: %d", sizeof(server_persistent_state_t));
    printf("\n\n\n\n");
}

void server_load_running_states_to_active_clients() {
    server_persistent_state_t server_persistent_state = {0};
    bool valid_crc = load_server_state(&server_persistent_state);

    if (valid_crc) {
        printf("LOADING ATTEMPT SUCCESSFULL! Loading states.\n");
        for (uint8_t index = 0; index < active_server_connections_number; index++) {
            server_load_client_state(active_uart_server_connections[index], &server_persistent_state);
        }
    } else {
        printf("LOADING ATTEMPT FAILED! Initializing Configuration\n");
        server_configure_persistent_state(&server_persistent_state);
    }

    //server_print_persistent_state(&server_persistent_state);
}
