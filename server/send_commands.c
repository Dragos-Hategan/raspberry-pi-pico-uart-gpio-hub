#include <string.h>
#include "pico/error.h"
#include "server.h"
#include <stdio.h>
#include "functions.h"

bool first_display = true;

void server_display_menu(void); 

void flush_stdin(){
    while (true){
        int ch = getchar_timeout_us(0);
        if (ch == PICO_ERROR_TIMEOUT) break;
    }
}

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

static bool read_uint32_immediate(uint32_t *out){
    flush_stdin();
    printf("\n> ");
    fflush(stdout);

    int ch = getchar();
    putchar(ch);         
    putchar('\n');

    if (ch >= '0' && ch <= '9') {
        *out = ch - '0';
        return true;
    }
    return false;
}

/**
 * @brief Prints all currently active UART connections to the console.
 *
 * Displays each valid UART connection with its associated TX/RX pins and UART instance number.
 */
static inline void server_display_active_clients(){    
    printf("These are the active connections:\n");
    for (uint8_t index = 1; index <= active_server_connections_number; index++){
        printf("%d. GPIO Pin Pair=[%d,%d]. UART Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        }
}

static inline void server_set_device_state(uart_pin_pair_t pin_pair, uart_inst_t* uart_instance, uint8_t gpio_index, bool device_state, uint32_t flash_client_index){
    server_send_device_state(pin_pair, uart_instance, gpio_index, device_state);
    
    server_persistent_state_t state_copy;
    memcpy(&state_copy, (const server_persistent_state_t *)SERVER_FLASH_ADDR, sizeof(state_copy));
    
    state_copy.clients[flash_client_index].running_client_state.devices[gpio_index > 22 ? (gpio_index - 3) : (gpio_index)].is_on = device_state;
    
    save_server_state(&state_copy);
}

static bool server_choose_client(uint32_t *client_index){
    if (active_server_connections_number == 1){
        *client_index = 1;
        return true;
    }
       
    for (uint8_t index = 0; index < active_server_connections_number; index++){
        printf("Client No. %d, connected to the server's GPIO pins [%d,%d]\n",
            index + 1,
            active_uart_server_connections[index].pin_pair.tx,
            active_uart_server_connections[index].pin_pair.rx);
    }

    printf("\nWhat client number do you want to access?");

    if (read_uint32_immediate(client_index) && *client_index >= 1 && *client_index <= active_server_connections_number){
        return true;
    }

    return false;
}

static bool server_choose_device(uint32_t *device_index, const client_state_t *running_client_state){  
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, running_client_state);
    }

    printf("\nWhat device number do you want to access?");

    if (read_uint32_line(device_index) && *device_index >= 1 && *device_index <= MAX_NUMBER_OF_GPIOS){
        if (running_client_state->devices[*device_index - 1].gpio_number != UART_CONNECTION_FLAG_NUMBER){
            return true;
        }else{
            printf("Selected device is used as UART connection.\n");
        }
    }

    return false;
}

static bool server_choose_state(bool *device_state){
    uint32_t state_number;
    printf("\nWhat state?\nON = 1\nOFF = 0");

    if (read_uint32_immediate(&state_number) && state_number >= 0 && state_number <= 1){
        *device_state = state_number ? true : false;
        return true;
    }

    return false;
}

static inline void print_input_error(){
    printf("Invalid input or overflow. Try again.\n");
}

static void server_read_client_and_device_data(){ 
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    uint32_t client_index;
    bool correct_client_input = false;
    while (!correct_client_input){
        if (server_choose_client(&client_index)){
            correct_client_input = true;
        }else{
            print_input_error();
        }
    }

    uint32_t flash_client_index;
    for (uint8_t index = 0; index < MAX_SERVER_CONNECTIONS; index++){
        if (active_uart_server_connections[client_index - 1].pin_pair.tx == flash_state->clients[index].uart_connection.pin_pair.tx){
            flash_client_index = index;
            break;
        }   
    }
    uint32_t device_index;
    bool correct_device_input = false;
    while (!correct_device_input){
        if (server_choose_device(&device_index, &flash_state->clients[flash_client_index].running_client_state)){
            correct_device_input = true;
        }else{
            print_input_error();
        }
    }

    bool device_state;
    bool correct_state_input = false;
    while (!correct_state_input){
        if (server_choose_state(&device_state)){
            correct_state_input = true;
        }else{
            print_input_error();
        }
    }

    server_set_device_state(active_uart_server_connections[client_index - 1].pin_pair,
        active_uart_server_connections[client_index - 1].uart_instance,
        flash_state->clients[flash_client_index].running_client_state.devices[device_index - 1].gpio_number,
        device_state,
        flash_client_index);
}

static void server_toggle_device(){
    
}

static void server_load_configuration(){

}

static void server_save_configuration(){
    
}

static void server_select_action(uint32_t choice){
    switch (choice){
        case 1:
            server_display_active_clients();
            break;
        case 2:
            server_read_client_and_device_data();
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
            break;
    }
}

static void server_read_choice(){
    uint32_t choice = 0;
    if (read_uint32_immediate(&choice)){
        server_select_action(choice);
    }
    else{
        print_input_error();
    }
}

void server_display_menu(){
    if (first_display){ 
        printf("\033[2J");    // delete screen
        printf("\033[H");     // move cursor to upper left screen
        first_display = false;

        printf("\n****************************************************\n\n");

        printf("Welcome!\n");
        server_display_active_clients();

        printf("\n");
    }

    printf(
        "Options:\n"
        "1. Display clients\n"
        "2. Set client's device\n"
        "3. Toggle client's device\n"
        "4. Load configuration\n"
        "5. Save configuration\n"
    );
        
    printf("\nPick an option");
    server_read_choice();

    printf("\n****************************************************\n\n");
}

void server_listen_for_commands(){
    while(true){
        if (stdio_usb_connected()){
            server_display_menu();
        }else{
            tight_loop_contents();
        }
    }
}