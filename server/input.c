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
 * or if an overflow would occur.
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
    printf("\n> ");
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
            printf("\b \b");
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

/**
 * @brief Displays a prompt and reads a number from the user in a given range.
 *
 * Continuously prompts the user with the provided message, then waits for a valid number within
 * the given range. Returns true only if input is valid and within [min, max].
 *
 * @param message Message to display to the user.
 * @param out Pointer to store the validated number.
 * @param min Minimum allowed value.
 * @param max Maximum allowed value.
 * @return true if valid number was entered and within range, false otherwise.
 */
bool read_user_choice_in_range(const char* message, uint32_t* out, uint32_t min, uint32_t max){
    printf("%s", message);
    if (read_uint32_line(out) && (*out >= min && *out <= max)){
        return true;
    }

    return false;
}
