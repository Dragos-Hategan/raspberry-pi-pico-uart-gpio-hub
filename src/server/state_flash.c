/**
 * @file state_flash.c
 * @brief Flash storage management for server persistent state on Raspberry Pi Pico.
 *
 * This file provides:
 * - CRC32 checksum computation for data integrity
 * - Functions to load and save the server's persistent state to internal flash
 * - Interrupt-safe flash programming using temporary buffers
 *
 * All operations ensure the data structure integrity is verified before being accepted
 * (via CRC32) and written atomically to avoid corruption.
 *
 * Functions in this file are used during boot, configuration changes, or when saving state.
 *
 */

#include <string.h> 

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#include "server.h"

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

bool load_server_state(server_persistent_state_t *out_state) {
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    memcpy(out_state, flash_state, sizeof(server_persistent_state_t));

    uint32_t saved_crc = out_state->crc;
    out_state->crc = 0;
    uint32_t computed_crc = compute_crc32(out_state, sizeof(server_persistent_state_t));
    out_state->crc = saved_crc;

    return (saved_crc == computed_crc);
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
