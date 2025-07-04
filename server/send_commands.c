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
static inline void server_display_active_clients(){
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

static uint32_t read_client_index(){
    char buffer[1];
    uint32_t len = 0;

    printf("\n> ");
    fflush(stdout);

    while (len < sizeof(buffer)) {
        int ch = getchar();
        buffer[len++] = (char)ch;
    }
    buffer[len] = '\0';

    return string_to_uint32(buffer, &choice);
}

static void server_set_device(){ //uint8_t gpio_number, bool state
    uint32_t client_index;

    printf("What device index do you waht to access?\n");
    for (uint8_t index = 0; index < active_server_connections_number; index++){
        printf("Device index = %d, connected to pins [%d,%d]\n", index + 1, active_uart_server_connections[index].pin_pair.tx, active_uart_server_connections[index].pin_pair.rx);
    }

    client_index = read_client_index();

    printf("%u\n", client_index);
    
    //uart_init_with_pins(uart, pin_pair, DEFAULT_BAUDRATE);

    // char msg[8];
    // snprintf(msg, sizeof(msg), "[%d,%d]", gpio_number, state);
    // uart_puts(uart, msg);
    // printf("Sent: %s\n", msg);
    // sleep_ms(10);

    // reset_gpio_pins(pin_pair);
}

static void server_toggle_device(){
    
}

static void server_load_configuration(){

}

static void server_save_configuration(){
    
}

static void server_select_action(){
    switch (choice){
        case 1:
            server_display_active_clients();
            break;
        case 2:
            server_set_device();
            break;
        case 3:
            server_toggle_device();
            break;
        case 4:
            server_load_configuration();
            break;
        case 5:
            server_save_configuration();
            break;
        default:
            printf("Out of range. Try again.\n");
            server_display_menu();
            break;
    }
}

static void server_read_choice(){
    char buffer[1];
    uint32_t len = 0;

    printf("\n> ");
    fflush(stdout);

    while (len < sizeof(buffer)) {
        int ch = getchar();
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
        "2. Set client's device\n"
        "3. Toggle client's device\n"
        "4. Load configuration\n"
        "5. Save configuration\n"
    );
        
    printf("\nPick an option");
    server_read_choice();
}

void server_listen_for_commands(){
    sleep_ms(2000);
    
    server_display_active_clients();

    while(true){
        server_display_menu();
    }
    
}