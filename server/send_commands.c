#include "server.h"
#include <stdio.h>
#include "functions.h"

bool first_display = true;
uint32_t choice = 0;

void server_display_menu(void); 

static bool string_to_uint32(const char *str, uint32_t *out) {
    uint32_t result = 0;

    // Skip leading spaces
    while (*str == ' ') str++;

    if (*str == '\0') return false; // empty string after spaces

    while (*str) {
        if (*str < '0' || *str > '9') {
            return false; // invalid character
        }

        uint8_t digit = *str - '0';

        // Check for overflow before multiplying
        if (result > (UINT32_MAX - digit) / 10) {
            return false; // would overflow
        }

        result = result * 10 + digit;
        str++;
    }

    *out = result;
    return true;
}

/**
 * @brief Prints all currently active UART connections to the console.
 *
 * Displays each valid UART connection with its associated TX/RX pins and UART instance number.
 */
static inline void print_active_connections(){
    if (first_display){ 
        printf("\033[2J");    // delete screen
        printf("\033[H");     // move cursor to upper left screen
        first_display = false;
        printf("Welcome!\n");
    }
    printf("These are the active connections:\n");
    for (uint8_t index = 1; index <= active_server_connections_number; index++){
        printf("%d. Pair=[%d,%d]. Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        }
    printf("\n");
}

static void server_select_action(){
    switch (choice){
        case 1:
            print_active_connections();
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            break;
        default:
            printf("Out of range. Try again.\n");
            server_display_menu();
            break;
    }
}

static void server_read_choice(){
    char buffer[32];
    uint32_t len = 0;

    printf("\n> ");
    fflush(stdout); // asigură afișarea promptului

    while (len < sizeof(buffer) - 1) {
        int ch = getchar();
        if (ch == '\r' || ch == '\n') {
            break;
        }
        buffer[len++] = (char)ch;
    }
    buffer[len] = '\0';

    printf("%s\n", buffer);

    if (string_to_uint32(buffer, &choice)){
        server_select_action();
    }
    else{
        printf("Invalid input or overflow. Try again.\n");
        server_display_menu();
    }
}

void server_display_menu(){
    printf(
        "1. Display clients\n"
        "2. Add device to client\n"
        "3. Remove device from client\n"
        "4. Set client's device state\n"
        "5. Toggle client's device state\n"
        "6. Load configuration\n"
        "7. Save configuration\n"
    );
        
    printf("\nPick an option");
    server_read_choice();
}

void server_listen_for_commands(){
    sleep_ms(8000);
    
    print_active_connections();

    while(true){
        server_display_menu();
    }
    
}