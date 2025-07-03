#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "hardware/flash.h"
#include "pico/stdlib.h"
#include "types.h"
#include "server.h"
#include "hardware/sync.h"
#include "functions.h"

#define SERVER_SECTOR_SIZE    4096
#define SERVER_PAGE_SIZE      256
#define SERVER_FLASH_OFFSET   (PICO_FLASH_SIZE_BYTES - SERVER_SECTOR_SIZE)
#define SERVER_FLASH_ADDR     (XIP_BASE + SERVER_FLASH_OFFSET)

// ---------------- CRC ----------------
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

// ------------- Flash IO ---------------
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

// ---------- UART Communication ----------
void send_default_client_state(uart_inst_t* uart_instance, uart_pin_pair_t pin_pair){
    uart_init_with_pins(uart_instance, pin_pair, DEFAULT_BAUDRATE);
    char msg[32];
    snprintf(msg, sizeof(msg), "["EMPTY_MEMORY_MESSAGE"]");
    uart_puts(uart_instance, msg);
    sleep_ms(5);
    reset_gpio_pins(pin_pair);
}

static void send_client_state(uart_pin_pair_t pin_pair, uart_inst_t* uart, const client_state_t* state){
    for (uint8_t i = 0; i < MAX_NUMBER_OF_ACTIVE_DEVICES; i++) {
        char msg[8];
        snprintf(msg, sizeof(msg), "[%d,%d]", state->devices[i].gpio_number, state->devices[i].is_on);
        uart_puts(uart, msg);
        sleep_ms(5);
    }
    reset_gpio_pins(pin_pair);
}

// ---------- Client Handling ----------
client_t* server_add_client(const uart_connection_t *conn, server_persistent_state_t *server_persistent_state) {
    for (uint8_t i = 0; i < MAX_SERVER_CONNECTIONS; i++) {
        client_t *existing = &server_persistent_state->clients[i];
        if (existing->uart_connection.pin_pair.tx == conn->pin_pair.tx &&
            existing->uart_connection.pin_pair.rx == conn->pin_pair.rx &&
            existing->uart_connection.uart_instance == conn->uart_instance) {
            return existing;
        }
    }

    for (uint8_t i = 0; i < MAX_SERVER_CONNECTIONS; i++) {
        client_t *slot = &server_persistent_state->clients[i];
        if (slot->uart_connection.uart_instance == NULL) {
            slot->uart_connection = *conn;
            return slot;
        }
    }
    return NULL; 
}

void server_add_new_clients(server_persistent_state_t *server_persistent_state) {
    for (uint8_t i = 0; i < active_server_connections_number; i++) {
        server_add_client(&active_uart_server_connections[i], server_persistent_state);
    }
    save_server_state(server_persistent_state);
}

// ---------- Load & Sync ----------
static void server_load_client_state(uart_connection_t uart_connection, server_persistent_state_t *server_persistent_state) {
    for (uint8_t i = 0; i < MAX_SERVER_CONNECTIONS; i++) {
        client_t *saved_client = &server_persistent_state->clients[i];

        if (saved_client->uart_connection.pin_pair.tx == uart_connection.pin_pair.tx &&
            saved_client->uart_connection.pin_pair.rx == uart_connection.pin_pair.rx &&
            saved_client->uart_connection.uart_instance == uart_connection.uart_instance) {

            send_client_state(uart_connection.pin_pair, uart_connection.uart_instance, &saved_client->running_client_state);
            return;
        }
    }

    // Nu a fost găsit → adaugăm
    client_t *new_client = server_add_client(&uart_connection, server_persistent_state);
    if (new_client != NULL) {
        save_server_state(server_persistent_state);
    }
}

void server_load_running_states_to_active_clients() {
    server_persistent_state_t server_persistent_state = {0};
    if (load_server_state(&server_persistent_state)) {
        for (uint8_t index = 0; index < active_server_connections_number; index++) {
            server_load_client_state(active_uart_server_connections[index], &server_persistent_state);
        }
    } else {
        server_add_new_clients(&server_persistent_state);
    }
}
