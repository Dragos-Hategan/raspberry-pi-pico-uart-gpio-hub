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
        printf("%d. GPIO Pin Pair=[%d,%d]. UART Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        }
    printf("\n");
}

static uint32_t client_index_input(){
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

    return string_to_uint32(buffer, &choice);
}

static inline void server_set_device_state(uart_pin_pair_t pin_pair, uart_inst_t* uart_instance){
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    printf("This is the running state of the client:\n");
    uint8_t client_index;
    for (client_index = 0; client_index < MAX_SERVER_CONNECTIONS; client_index++){
        if (pin_pair.tx == flash_state->clients[client_index].uart_connection.pin_pair.tx){
            client_t client = flash_state->clients[client_index];
            server_print_running_client_state(&client);
            break;
        }   
    }
}

static void server_choose_client(){ 
    
    if (active_server_connections_number == 1){
        server_set_device_state(active_uart_server_connections[0].pin_pair, active_uart_server_connections[0].uart_instance);
    }else{
        uint32_t client_index;

        printf("What device number do you waht to access?\n");
        for (uint8_t index = 0; index < active_server_connections_number; index++){
            printf("Device No. %d, connected to GPIO pins [%d,%d]\n", index + 1, active_uart_server_connections[index].pin_pair.tx, active_uart_server_connections[index].pin_pair.rx);
        }
        
        if (client_index_input() && choice >= 1 && choice <= active_server_connections_number){
            server_set_device_state(active_uart_server_connections[choice - 1].pin_pair, active_uart_server_connections[choice - 1].uart_instance);
        }else{
            printf("Invalid input or overflow. Try again.\n");
            server_choose_client();
        }
        
        printf("%u\n", choice);
    }

    


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
            server_choose_client();
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