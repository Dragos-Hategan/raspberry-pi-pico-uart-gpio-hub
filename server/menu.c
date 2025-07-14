/**
 * @file menu.c
 * @brief USB CLI menu logic for controlling clients on the UART server.
 *
 * Provides a terminal-based interface to:
 * - Display current client connections.
 * - Select and control GPIO states of client devices.
 * - Toggle GPIO states (planned).
 * - Load/save configurations (planned).
 * 
 * Input is read from USB serial, with range validation and error handling.
 */

#include <string.h>

#include "input.h"
#include "server.h"

bool first_display = true;

void server_display_menu(void);

static inline void print_input_error(void){
    printf("Invalid input or overflow. Try again.\n");
}

static inline void print_cancel_message(void){
    printf("0. cancel\n");
}

static inline void print_delimitor(void){
    printf("\n****************************************************\n\n");
}

/**
 * @brief Prompts the user to enter a desired state: ON (1) or OFF (0).
 *
 * @param device_state Pointer to store the selected boolean state.
 * @return true if valid input received, false otherwise.
 */
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

/**
 * @brief Prompts the user to select a GPIO device to modify.
 *
 * @param device_index Output pointer for selected index (1-based).
 * @param running_client_state The state structure of the selected client.
 * @return true if valid device selected, false otherwise.
 */
static bool choose_device(uint32_t *device_index, const client_state_t *running_client_state){  
    for (uint8_t gpio_index = 0; gpio_index < MAX_NUMBER_OF_GPIOS; gpio_index++){
        server_print_gpio_state(gpio_index, running_client_state);
    }

    const uint32_t INPUT_MIN_DEVICE_INDEX = 0;
    const uint32_t INPUT_MAX_DEVICE_INDEX = MAX_NUMBER_OF_GPIOS;
    const char *MESSAGE = "\nWhat device number do you want to access?";

    print_cancel_message();

    if (read_user_choice_in_range(MESSAGE, device_index, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        if (running_client_state->devices[*device_index - 1].gpio_number != UART_CONNECTION_FLAG_NUMBER){
            return true;
        }else{
            printf("Selected device is used as UART connection.\n");
        }
    }

    return false;
}

/**
 * @brief Prompts the user to choose which connected client to access.
 *
 * @param client_index Output pointer for selected index (1-based).
 * @return true if valid client selected, false otherwise.
 */
static bool choose_client(uint32_t *client_index){
    if (active_server_connections_number == 1){
        *client_index = 1;
        return true;
    }
       
    for (uint32_t index = 0; index < active_server_connections_number; index++){
        printf("%u. Client No. %u, connected to the server's GPIO pins [%d,%d]\n",
            index + 1,
            index + 1,
            active_uart_server_connections[index].pin_pair.tx,
            active_uart_server_connections[index].pin_pair.rx);
    }

    const uint32_t INPUT_MIN_DEVICE_INDEX = 0;
    const uint32_t INPUT_MAX_DEVICE_INDEX = active_server_connections_number;
    const char *MESSAGE = "\nWhat client number do you want to access?";

    print_cancel_message();

    if (read_user_choice_in_range(MESSAGE, client_index, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        return true;
    }

    return false;
}

/**
 * @brief Finds the corresponding flash client index for a given active client.
 *
 * Compares the TX pin of the selected client with entries in the flash structure to
 * determine the matching flash client index.
 *
 * @param flash_client_index Output pointer to store the resolved flash index.
 * @param client_index Index of the selected client (1-based).
 * @param flash_state Pointer to the flash-stored server state.
 */
static void find_corect_client_index_from_flash(uint32_t *flash_client_index, uint32_t client_index, const server_persistent_state_t *flash_state){
    for (uint8_t index = 0; index < MAX_SERVER_CONNECTIONS; index++){
        if (active_uart_server_connections[client_index - 1].pin_pair.tx == flash_state->clients[index].uart_connection.pin_pair.tx){
            *flash_client_index = index;
            break;
        }   
    }
}

/**
 * @brief Reads a valid ON/OFF state input from the user.
 *
 * Prompts until a valid binary state is selected (0 or 1).
 *
 * @param device_state Output pointer to store the selected state (true = ON, false = OFF).
 */
static void read_device_state(bool *device_state){
    bool correct_state_input = false;
    while (!correct_state_input){
        if (choose_state(device_state)){
            correct_state_input = true;
        }else{
            print_input_error();
        }
    }
}

/**
 * @brief Prompts the user to select a device index for the given client.
 *
 * Repeats until a valid device is selected or cancel (0) is entered.
 *
 * @param device_index Output pointer to store the selected device index (1-based).
 * @param flash_client_index Index of the client in the flash state structure.
 * @param flash_state Pointer to the flash-stored server state.
 */
static void read_device_index(uint32_t *device_index, uint32_t flash_client_index, const server_persistent_state_t *flash_state){
    bool correct_device_input = false;
    while (!correct_device_input){
        if (choose_device(device_index, &flash_state->clients[flash_client_index].running_client_state)){
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

/**
 * @brief Prompts the user to select a client index.
 *
 * Repeats until a valid client is selected or cancel (0) is entered.
 *
 * @param client_index Output pointer to store the selected client index (1-based).
 */
static void read_client_index(uint32_t *client_index){ 
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

/**
 * @brief Reads client and device selections and resolves their flash indexes.
 *
 * Guides the user through selecting a client and a device,
 * then determines their corresponding indexes in the flash state structure.
 *
 * @param client_index Output pointer to store the selected client index (1-based).
 * @param flash_state Pointer to the flash-stored server state.
 * @param flash_client_index Output pointer to store the resolved flash client index.
 * @param device_index Output pointer to store the selected device index (1-based).
 */
static void get_client_flash_device_indexes(uint32_t *client_index, const server_persistent_state_t * flash_state, uint32_t *flash_client_index, uint32_t *device_index){
    read_client_index(client_index);
    if (!*client_index){
        return;
    }
    find_corect_client_index_from_flash(flash_client_index, *client_index, flash_state);
    read_device_index(device_index, *flash_client_index, flash_state);
}

static void delete_configuration(void){

}

/**
 * @brief Placeholder for loading saved client configurations.
 * (Planned feature)
 */
static void load_configuration(void){

}

/**
 * @brief Saves the current running configuration of a client into a preset slot.
 *
 * Loads the full persistent server state from flash, copies the active running state
 * of the specified client into the selected preset configuration slot, and saves
 * the updated structure back to flash.
 *
 * @param flash_configuration_index Index of the target preset configuration [0..(NUMBER_OF_POSSIBLE_PRESETS - 1)].
 * @param flash_client_index Index of the client within the persistent flash state.
 */
static void save_running_configuration_into_preset_configuration(uint32_t flash_configuration_index, uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    memcpy(
        &state.clients[flash_client_index].preset_configs[flash_configuration_index],
        &state.clients[flash_client_index].running_client_state,
        sizeof(client_state_t)
    );
    
    save_server_state(&state);
    printf("Configuration saved in Preset[%u]!\n", flash_configuration_index + 1);
}

/**
 * @brief Prompts the user to select a preset slot and saves the running configuration there.
 *
 * Displays all preset configuration slots for the selected client and asks the user to choose one.
 * If the user confirms, it delegates to `save_running_configuration_into_preset_configuration()` 
 * to perform the actual copy and flash save.
 *
 * @param flash_client_index Index of the client in the persistent flash state structure.
 */
static void save_running_configuration(uint32_t flash_client_index){
    for (uint32_t confiuration_index = 1; confiuration_index <= NUMBER_OF_POSSIBLE_PRESETS; confiuration_index++){
        printf("%u. Preset Config[%u]\n", confiuration_index, confiuration_index);
    }

    uint32_t flash_configuration_index;
    const uint32_t INPUT_MIN_DEVICE_INDEX = 0;
    const uint32_t INPUT_MAX_DEVICE_INDEX = NUMBER_OF_POSSIBLE_PRESETS;
    const char *MESSAGE = "\nWhere will the running configuration be saved?";

    print_cancel_message();
    
    if (read_user_choice_in_range(MESSAGE, &flash_configuration_index, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        if (flash_configuration_index == 0){
            return;
        }else{
            save_running_configuration_into_preset_configuration(flash_configuration_index - 1, flash_client_index);
        }
    }
    else{
        print_input_error();
    }
}

static void build_configuration(){
    printf("Building configuration\n");
}

/**
 * @brief Handles the "Save configuration" menu option.
 *
 * - Prompts the user to select a client.
 * - Displays current running configuration and saved presets for that client.
 * - Asks how the configuration should be saved (running or built).
 * - Based on user input, either saves the current state or enters build mode.
 */
static void save_configuration(void){
    uint32_t client_index;
    read_client_index(&client_index);
    if (!client_index){
        return;
    }

    uint32_t flash_client_index;
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    find_corect_client_index_from_flash(&flash_client_index, client_index, flash_state);

    const client_t *client = &flash_state->clients[flash_client_index];
    server_print_running_client_state(client);
    for (uint8_t client_preset_index = 0; client_preset_index < NUMBER_OF_POSSIBLE_PRESETS; client_preset_index++){
        server_print_client_preset_configuration(client, client_preset_index);
    }

    printf("\n1. Save running configuration.\n2. Build and save configuration.\n");

    uint32_t option;
    const uint32_t INPUT_MIN_DEVICE_INDEX = 0;
    const uint32_t INPUT_MAX_DEVICE_INDEX = 2;
    const char *MESSAGE = "\nHow do you want to save?";

    print_cancel_message();
    
    if (read_user_choice_in_range(MESSAGE, &option, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        if (option == 0){
            return;
        }else if (option == 1){
            save_running_configuration(flash_client_index);
        }else{
            build_configuration();
        }
    }
    else{
        print_input_error();
    }
}

/**
 * @brief Toggles the state of a selected GPIO device for a selected client.
 *
 * Prompts the user to select a client and one of its devices, then reads the current
 * state of the device from flash and flips it (ON → OFF, OFF → ON).
 * Sends the updated state to the corresponding client via UART, and updates the flash accordingly.
 */
static void toggle_device(void){
    uint32_t client_index;
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    uint32_t flash_client_index;
    uint32_t device_index;
    get_client_flash_device_indexes(&client_index, flash_state, &flash_client_index, &device_index);

    if (!client_index || !device_index){
        return;
    }

    uint32_t gpio_index = flash_state->clients[flash_client_index].running_client_state.devices[device_index - 1].gpio_number;
    bool device_state = flash_state->clients[flash_client_index].
                        running_client_state.
                        devices[gpio_index > 22 ? (gpio_index - 3) : (gpio_index)].
                        is_on ? false : true;

    server_set_device_state_and_update_flash(active_uart_server_connections[client_index - 1].pin_pair,
    active_uart_server_connections[client_index - 1].uart_instance,
    gpio_index,
    device_state,
    flash_client_index);
}

/**
 * @brief Sets the state of a selected GPIO device for a selected client.
 *
 * Prompts the user to select a client, one of its devices, and a desired state (ON/OFF),
 * then sends the state to the client via UART and updates the flash accordingly.
 */
static void set_client_device(void){    
    uint32_t client_index;
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    uint32_t flash_client_index;
    uint32_t device_index;
    get_client_flash_device_indexes(&client_index, flash_state, &flash_client_index, &device_index);

    if (!client_index || !device_index){
        return;
    }

    bool device_state;
    read_device_state(&device_state);
    uint32_t gpio_index = flash_state->clients[flash_client_index].
                        running_client_state.
                        devices[device_index - 1].
                        gpio_number;

    server_set_device_state_and_update_flash(active_uart_server_connections[client_index - 1].pin_pair,
        active_uart_server_connections[client_index - 1].uart_instance,
        gpio_index,
        device_state,
        flash_client_index);
}

/**
 * @brief Prints all currently active UART connections to the console.
 *
 * Displays each valid UART connection with its associated TX/RX pins and UART instance number.
 */
static inline void display_active_clients(void){    
    printf("These are the active connections:\n");
    for (uint8_t index = 1; index <= active_server_connections_number; index++){
        printf("%d. GPIO Pin Pair=[%d,%d]. UART Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        }
}

/**
 * @brief Dispatches the user-selected menu option to the corresponding action.
 *
 * @param choice The selected menu number.
 */
static void select_action(uint32_t choice){
    switch (choice){
        case 1:
            display_active_clients();
            break;
        case 2:
            set_client_device();
            break;
        case 3:
            toggle_device();
            break;
        case 4:
            save_configuration();
            break;
        case 5:
            load_configuration();
            break;
        case 6:
            delete_configuration();
            break;
        default:
            printf("Out of range. Try again.\n");
            break;
    }
}


/**
 * @brief Reads a menu selection from the user and processes it.
 */
static void server_read_choice(void){
    uint32_t option;
    const uint32_t INPUT_MIN_DEVICE_INDEX = 1;
    const uint32_t INPUT_MAX_DEVICE_INDEX = 6;
    const char *MESSAGE = "\nPick an option";
    
    if (read_user_choice_in_range(MESSAGE, &option, INPUT_MIN_DEVICE_INDEX, INPUT_MAX_DEVICE_INDEX)){
        select_action(option);
    }
    else{
        print_input_error();
    }
}

/**
 * @brief Entry point for the USB menu system.
 *
 * Prints a welcome screen on first call, then shows the menu options.
 * Processes user selection via `server_read_choice()`.
 */
void server_display_menu(void){
    if (first_display){ 
        printf("\033[2J");    // delete screen
        printf("\033[H");     // move cursor to upper left screen
        first_display = false;

        print_delimitor();

        printf("Welcome!\n");
        display_active_clients();

        printf("\n");
    }

    printf(
        "Options:\n"
        "1. Display clients\n"
        "2. Set client's device\n"
        "3. Toggle client's device\n"
        "4. Save configuration\n"
        "5. Load configuration\n"
        "6. Delete configuration\n"
    );
    server_read_choice();

    print_delimitor();
}
