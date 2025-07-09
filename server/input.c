#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pico/stdio.h"
#include "pico/error.h"

#include "input.h"

static void flush_stdin(){
    while (true){
        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) break;
    }
}

static bool string_to_uint32(const char *str, uint32_t *out) {
    uint32_t result = 0;

    while (*str == ' ') str++;

    if (*str == '\0') return false;      

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

static bool read_uint32_line(uint32_t *out){
    flush_stdin();
    printf("\n> ");
    fflush(stdout);
    
    char buffer[12] = {0};
    int len = 0;

    while (true){
        int ch = getchar();
        if (ch == '\r' || ch == '\n') 
            break;
        if (len < (int)(sizeof(buffer) - 1)) {
            buffer[len++] = (char)ch;
            putchar(ch);
        }
    }
    putchar('\n');
    return string_to_uint32(buffer, out);
}

bool read_user_choice_in_range(const char* message, uint32_t* out, uint32_t min, uint32_t max){
    printf("%s", message);
    if (read_uint32_line(out) && *out >= min && *out <= max){
        return true;
    }
    return false;
}
