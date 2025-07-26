/**
 * @file input.c
 * @brief Implements utility functions for user input via USB CLI.
 *
 * This module provides routines for reading and validating numeric input from
 * the user, typically through a serial USB interface. It includes internal
 * helpers for flushing input, parsing strings into integers, and enforcing
 * valid ranges.
 *
 * Used by the server for CLI interaction with the user (e.g., GPIO selection).
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pico/stdio.h"
#include "pico/error.h"

#include "input.h"
#include "server.h"
#include "menu.h"

/**
 * @brief Clears any characters from the input buffer.
 *
 * Useful to prevent leftover characters from interfering with new input.
 */
static void flush_stdin(void){
    while (true){
        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) break;
    }
}

/**
 * @brief Converts a numeric string to an unsigned 32-bit integer.
 *
 * Ignores leading spaces. Returns false if the string is empty, contains non-digit characters,
 * contains leading zeros or if an overflow would occur.
 *
 * @param str Pointer to the input string.
 * @param out Pointer to the variable where the result will be stored.
 * @return true if the conversion was successful, false otherwise.
 */
static bool string_to_uint32(const char *str, uint32_t *out) {
    uint32_t result = 0;

    while (*str == ' ') str++;

    if (*str == '\0') return false;      

    if (str[0] == '0' && str[1] != '\0') return false;

    while (*str) {
        if (*str < '0' || *str > '9') {
            return false;
        }

        uint8_t digit = *str - '0';
        if (result > (UINT32_MAX - digit) / 10) {
            return false;
        }

        result = result * 10 + digit;
        str++;
    }

    *out = result;
    return true;
}

/**
 * @brief Reads an unsigned integer from standard input (stdin).
 *
 * Flushes any previous characters, then reads input until newline (`\n` or `\r`) or buffer limit.
 * Calls `string_to_uint32()` to parse the result.
 *
 * @param out Pointer to store the parsed result.
 * @return true if parsing was successful and result is a valid uint32_t, false otherwise.
 */
static bool read_uint32_line(uint32_t *out){
    flush_stdin();
    print_and_update_buffer("\n> ");
    fflush(stdout);
    
    char buffer[12] = {0};
    int len = 0;

    while (true){
        int ch = getchar();
        if ((ch == '\r' || ch == '\n') && len > 0) 
            break;
        
        if ((ch == 8 || ch == 127) && len > 0) {  // 8 = BS, 127 = DEL
            len--;
            buffer[len] = '\0';
            print_and_update_buffer("\b \b");
            continue;
        }
    
        if (ch >= '0' && ch <= '9'){
            if (len < (int)(sizeof(buffer) - 1)) {
                buffer[len++] = (char)ch;
                buffer[len] = '\0';
                putchar(ch);
            }
        }
    }
    putchar('\n');
    return string_to_uint32(buffer, out);
}

bool read_user_choice_in_range(const char* message, uint32_t* out, uint32_t min, uint32_t max){
    print_and_update_buffer(message);
    if (read_uint32_line(out) && (*out >= min && *out <= max)){
        return true;
    }

    return false;
}

bool choose_menu_option(uint32_t *menu_option){
    const char *MESSAGE = "\nPick an option";
    if (read_user_choice_in_range(MESSAGE, menu_option, MINIMUM_MENU_OPTION_INDEX_INPUT, MAXIMUM_MENU_OPTION_INDEX_INPUT)){
        return true;
    }
    return false;
}

bool choose_reset_variant(uint32_t *reset_variant){
    print_and_update_buffer("1. Running State.\n2. Preset Config.\n3. All Client Data.\n");
    
    const char *MESSAGE = "\nWhat do you want to reset?";
    print_cancel_message();
    if (read_user_choice_in_range(MESSAGE, reset_variant, MINIMUM_RESET_VARIANT_INPUT, MAXIMUM_RESET_VARIANT_INPUT)){
        return true;
    }

    return false;
}

void read_reset_variant(uint32_t *reset_variant){
    bool correct_reset_variant_input = false;
    while (!correct_reset_variant_input){
        if (choose_reset_variant(reset_variant)){
            if (*reset_variant == 0){
                return;
            }else{
                correct_reset_variant_input = true;
            }
        }else{
            print_input_error();
            print_and_update_buffer("\n");
        }
    }
}

bool choose_flash_configuration_index(uint32_t *flash_configuration_index){
    const char *MESSAGE = "\nWhat configuration do you want to access?";
    print_cancel_message();
    if (read_user_choice_in_range(MESSAGE, flash_configuration_index, MINIMUM_FLASH_CONFIGURATION_INDEX_INPUT, MAXIMUM_FLASH_CONFIGURATION_INDEX_INPUT)){
        return true;
    }
    return false;
}

void read_flash_configuration_index(uint32_t *flash_configuration_index){
    bool correct_flash_configuration_input = false;
    while (!correct_flash_configuration_input){
        for (uint32_t configuration_index = 1; configuration_index <= NUMBER_OF_POSSIBLE_PRESETS; configuration_index++){
            char string[BUFFER_MAX_STRING_SIZE];
            snprintf(string, sizeof(string), "%u. Preset Config[%u]\n", configuration_index, configuration_index);
            print_and_update_buffer(string);
        }
        if (choose_flash_configuration_index(flash_configuration_index)){
            correct_flash_configuration_input = true;
        }else{
            print_input_error();
            print_and_update_buffer("\n");
        }
    }
}

bool choose_state(uint32_t *device_state){
    print_and_update_buffer("\n1. ON\n2. OFF\n");
    const char *MESSAGE = "\nWhat state?";
    print_cancel_message();
    if (read_user_choice_in_range(MESSAGE, device_state, MINIMUM_DEVICE_STATE_INPUT, MAXIMUM_DEVICE_STATE_INPUT)){
        return true;
    }

    return false;
}

void read_device_state(uint32_t *device_state){
    bool correct_state_input = false;
    while (!correct_state_input){
        if (choose_state(device_state)){
            correct_state_input = true;
        }else{
            print_input_error();
        }
    }
}

bool choose_client(uint32_t *client_index){
    if (active_server_connections_number == 1){
        *client_index = 1;
        return true;
    }
    
    print_and_update_buffer("\n");

    for (uint32_t index = 0; index < active_server_connections_number; index++){
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "%u. Client No. %u, connected to the server's GPIO pins [%d,%d]\n",
            index + 1,
            index + 1,
            active_uart_server_connections[index].pin_pair.tx,
            active_uart_server_connections[index].pin_pair.rx);
        print_and_update_buffer(string);
    }

    const char *MESSAGE = "\nWhat client do you want to access?";
    print_cancel_message();
    if (read_user_choice_in_range(MESSAGE, client_index, MINIMUM_CLIENT_INDEX_INPUT, active_server_connections_number)){
        return true;
    }
    return false;
}

void read_client_index(uint32_t *client_index){ 
    bool correct_client_input = false;
    while (!correct_client_input){
        if (choose_client(client_index)){
            if (*client_index == 0){
                return;
            }else{
                correct_client_input = true;
            }
        }else{
            print_input_error();
        }
    }
}

bool choose_device(uint32_t *device_index, const client_state_t *client_state){  
    print_and_update_buffer("\n");
    server_print_state_devices(client_state);

    const char *MESSAGE = "\nWhat device number do you want to access?";
    print_cancel_message();
    if (read_user_choice_in_range(MESSAGE, device_index, MINIMUM_DEVICE_INDEX_INPUT, MAXIMUM_DEVICE_INDEX_INPUT)){
        if (client_state->devices[*device_index - 1].gpio_number != UART_CONNECTION_FLAG_NUMBER){
            return true;
        }else{
            print_and_update_buffer("\nSelected device is used as UART connection.\n");
        }
    }

    return false;
}

void read_device_index(uint32_t *device_index, uint32_t flash_client_index, const server_persistent_state_t *flash_state, const client_state_t *client_state){
    bool correct_device_input = false;
    while (!correct_device_input){
        if (choose_device(device_index, client_state)){
            if (*device_index == 0){
                return;
            }else{
                correct_device_input = true;
            }
        }else{
            print_input_error();
        }
    }
}

bool read_client_data(input_client_data_t *client_data, client_input_flags_t client_input_flags){
    if (client_input_flags.need_client_index){
        read_client_index(&client_data->client_index);
        if (!client_data->client_index){
            return false;
        }  

    }

    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    find_corect_client_index_from_flash(&client_data->flash_client_index, client_data->client_index, flash_state);
    const client_state_t *client_state = &flash_state->clients[client_data->flash_client_index].running_client_state;
    client_data->client_state = client_state;
    client_data->flash_state = flash_state;

    if (client_input_flags.need_device_index){
        read_device_index(&client_data->device_index, client_data->flash_client_index, flash_state, client_state);
        if (!client_data->device_index){
            return false;
        }
    }

    if (client_input_flags.need_device_state){
        read_device_state(&client_data->device_state);
        if (!client_data->device_state){
            return false;
        }
        client_data->device_state %= 2;
    }

    if (client_input_flags.need_config_index && !client_input_flags.is_building_preset){
        print_and_update_buffer("\n");
        server_print_running_client_state((const client_t *)&flash_state->clients[client_data->flash_client_index]);
        print_and_update_buffer("\n");
        server_print_client_preset_configurations((const client_t *)&flash_state->clients[client_data->flash_client_index]);

        read_flash_configuration_index(&client_data->flash_configuration_index);  
        if (!client_data->flash_configuration_index){
            return false;
        }
    }

    if (client_input_flags.is_building_preset){
        print_and_update_buffer("\n");
        server_print_client_preset_configurations((const client_t *)&flash_state->clients[client_data->flash_client_index]);   

        read_flash_configuration_index(&client_data->flash_configuration_index);  
        if (!client_data->flash_configuration_index){
            return false;
        }
        set_configuration_devices(client_data->flash_client_index, client_data->flash_configuration_index - 1, client_data);
    }

    if(client_input_flags.is_load){
        print_and_update_buffer("\n");
        server_print_running_client_state((const client_t *)&flash_state->clients[client_data->flash_client_index]);
        print_and_update_buffer("\n");
        server_print_client_preset_configurations((const client_t *)&flash_state->clients[client_data->flash_client_index]);

        read_flash_configuration_index(&client_data->flash_configuration_index);    
        if (!client_data->flash_configuration_index){
            return false;
        }
    }

    if (client_input_flags.need_reset_choice){
        print_and_update_buffer("\n");
        server_print_running_client_state((const client_t *)&flash_state->clients[client_data->flash_client_index]);
        print_and_update_buffer("\n");
        server_print_client_preset_configurations((const client_t *)&flash_state->clients[client_data->flash_client_index]);

        read_reset_variant(&client_data->reset_choice);
        if (!client_data->reset_choice){
            return false;
        }

        if (client_data->reset_choice == 2){
            print_and_update_buffer("\n");
            read_flash_configuration_index(&client_data->flash_configuration_index);  
            if (!client_data->flash_configuration_index){
                return false;
            }
        }
    }

    return true;
}