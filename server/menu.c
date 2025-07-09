#include "input.h"
#include "server.h"

bool first_display = true;

static inline void print_input_error(){
    printf("Invalid input or overflow. Try again.\n");
}

static void save_configuration(){
    
}

static void load_configuration(){

}

static void toggle_device(){
    
}

static bool choose_state(bool *device_state){
    uint32_t state_number;
    const uint32_t INPUT_MIN_DEVICE_INDEX = 0;
    const uint32_t INPUT_MAX_DEVICE_INDEX = 1;
    const char *MESSAGE = "\nWhat state?\nON = 1\nOFF = 0";

    if (read_user_choice_in_range(MESSAGE, &state_number, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        *device_state = state_number ? true : false;
        return true;
    }

    return false;
}

static bool choose_device(uint32_t *device_index, const client_state_t *running_client_state){  
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, running_client_state);
    }

    const uint32_t INPUT_MIN_DEVICE_INDEX = 1;
    const uint32_t INPUT_MAX_DEVICE_INDEX = MAX_NUMBER_OF_GPIOS;
    const char *MESSAGE = "\nWhat device number do you want to access?";

    if (read_user_choice_in_range(MESSAGE, device_index, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        if (running_client_state->devices[*device_index - 1].gpio_number != UART_CONNECTION_FLAG_NUMBER){
            return true;
        }else{
            printf("Selected device is used as UART connection.\n");
        }
    }

    return false;
}

static bool choose_client(uint32_t *client_index){
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

    const uint32_t INPUT_MIN_DEVICE_INDEX = 1;
    const uint32_t INPUT_MAX_DEVICE_INDEX = active_server_connections_number;
    const char *MESSAGE = "\nWhat client number do you want to access?";

    if (read_user_choice_in_range(MESSAGE, client_index, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        return true;
    }

    return false;
}

static void read_client_and_device_data(){ 
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    uint32_t client_index;
    bool correct_client_input = false;
    while (!correct_client_input){
        if (choose_client(&client_index)){
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
        if (choose_device(&device_index, &flash_state->clients[flash_client_index].running_client_state)){
            correct_device_input = true;
        }else{
            print_input_error();
        }
    }

    bool device_state;
    bool correct_state_input = false;
    while (!correct_state_input){
        if (choose_state(&device_state)){
            correct_state_input = true;
        }else{
            print_input_error();
        }
    }

    server_set_device_state_and_update_flash(active_uart_server_connections[client_index - 1].pin_pair,
        active_uart_server_connections[client_index - 1].uart_instance,
        flash_state->clients[flash_client_index].running_client_state.devices[device_index - 1].gpio_number,
        device_state,
        flash_client_index);
}

/**
 * @brief Prints all currently active UART connections to the console.
 *
 * Displays each valid UART connection with its associated TX/RX pins and UART instance number.
 */
static inline void display_active_clients(){    
    printf("These are the active connections:\n");
    for (uint8_t index = 1; index <= active_server_connections_number; index++){
        printf("%d. GPIO Pin Pair=[%d,%d]. UART Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        }
}

static void select_action(uint32_t choice){
    switch (choice){
        case 1:
            display_active_clients();
            break;
        case 2:
            read_client_and_device_data();
            break;
        case 3:
            toggle_device();
            break;
        case 4:
            load_configuration();
            break;
        case 5:
            save_configuration();
            break;
        default:
            printf("Out of range. Try again.\n");
            break;
    }
}

static void server_read_choice(){
    uint32_t option;
    const uint32_t INPUT_MIN_DEVICE_INDEX = 1;
    const uint32_t INPUT_MAX_DEVICE_INDEX = 5;
    const char *MESSAGE = "\nPick an option";
    
    if (read_user_choice_in_range(MESSAGE, &option, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        select_action(option);
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
        display_active_clients();

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
    server_read_choice();

    printf("\n****************************************************\n\n");
}
