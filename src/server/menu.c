/**
 * @file menu.c
 * @brief USB CLI menu logic for controlling clients on the UART server.
 *
 * Provides a terminal-based interface to:
 * - Display current client connections.
 * - Select and control GPIO states of client devices.
 * - Toggle GPIO states.
 * - Save/build/load/reset configurations.
 * 
 * Input is read from USB serial, with range validation and error handling.
 */

#include <string.h>

#include "hardware/watchdog.h"

#include "input.h"
#include "server.h"
#include "menu.h"

bool first_display = true;

void server_display_menu(void);

static void restart_application(){
    signal_reset_for_all_clients();
    watchdog_reboot(0,0,0);
}

/**
 * @brief Entry point for resetting client data.
 *
 * - Prompts the user to select a client.
 * - Displays its running and preset configurations.
 * - Asks the user what kind of reset to perform.
 * - Executes the corresponding reset action:
 *     - 1: Only running configuration
 *     - 2: Only preset configurations
 *     - 3: Entire client data (running + presets)
 */
static void reset_configuration(void){
    input_client_data_t input_client_data = {0};
    client_input_flags_t client_input_flags = {0};
    client_input_flags.need_client_index = true;
    client_input_flags.need_reset_choice = true;

    if (read_client_data(&input_client_data, client_input_flags)){
        if (input_client_data.reset_choice == 1){
            reset_running_configuration(input_client_data.flash_client_index);
        }else if (input_client_data.reset_choice == 2){
            reset_preset_configuration(input_client_data.flash_client_index, input_client_data.flash_configuration_index);
        }else{
            reset_all_client_data(input_client_data.flash_client_index);
        }
    }
}

/**
 * @brief Loads a saved preset configuration into a client's running state.
 *
 * - Prompts the user to select a client.
 * - Displays current running and preset configurations.
 * - Asks the user to select which preset to load.
 * - Loads and applies the selected preset into the running state.
 */
static void load_configuration(void){
    input_client_data_t input_client_data = {0};
    client_input_flags_t client_input_flags = {0};
    client_input_flags.need_client_index = true;
    client_input_flags.is_load = true;
    
    if (read_client_data(&input_client_data, client_input_flags)){
        load_configuration_into_running_state(input_client_data.flash_configuration_index - 1, input_client_data.flash_client_index);
    }
}

/**
 * @brief Starts an interactive process to build a client preset configuration.
 *
 * - Displays all existing preset configurations for the selected client.
 * - Prompts the user to choose one of the preset slots to modify.
 * - Calls `set_configuration_devices()` to configure the ON/OFF state of individual devices.
 *
 */
static void build_preset_configuration(void){
    input_client_data_t input_client_data = {0};
    client_input_flags_t client_input_flags = {0};
    client_input_flags.need_client_index = true;
    client_input_flags.is_building_preset = true;
    read_client_data(&input_client_data, client_input_flags);

    printf("\nBuilding Configuration Complete.\n");
}

/**
 * @brief Prompts the user to select a preset slot and saves the running configuration there.
 *
 * Displays all preset configuration slots for the selected client and asks the user to choose one.
 * If the user confirms, it delegates to `save_running_configuration_into_preset_configuration()` 
 * to perform the actual copy and flash save.
 *
 */
static void save_running_state(void){    
    input_client_data_t input_client_data = {0};
    client_input_flags_t client_input_flags = {0};
    client_input_flags.need_client_index = true;
    client_input_flags.need_config_index = true;

    if (read_client_data(&input_client_data, client_input_flags)){
        save_running_configuration_into_preset_configuration(input_client_data.flash_configuration_index - 1, input_client_data.flash_client_index);
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
    input_client_data_t input_client_data = {0};
    client_input_flags_t client_input_flags = {0};
    client_input_flags.need_client_index = true;
    client_input_flags.need_device_index = true;

    if (read_client_data(&input_client_data, client_input_flags)){
        uint32_t gpio_index = input_client_data.client_state->devices[input_client_data.device_index - 1].gpio_number;
        bool device_state = input_client_data.client_state->
                            devices[gpio_index > 22 ? (gpio_index - 3) : (gpio_index)].
                            is_on ? false : true;
    
        server_set_device_state_and_update_flash(active_uart_server_connections[input_client_data.client_index - 1].pin_pair,
        active_uart_server_connections[input_client_data.client_index - 1].uart_instance,
        gpio_index,
        device_state,
        input_client_data.flash_client_index);
    
        printf("\nDevice[%u] Toggled.\n", input_client_data.device_index);
    }
}

/**
 * @brief Sets the state of a selected GPIO device for a selected client.
 *
 * Prompts the user to select a client, one of its devices, and a desired state (ON/OFF),
 * then sends the state to the client via UART and updates the flash accordingly.
 */
static void set_client_device(void){    
    input_client_data_t input_client_data = {0};
    client_input_flags_t client_input_flags = {0};
    client_input_flags.need_client_index = true;
    client_input_flags.need_device_index = true;
    client_input_flags.need_device_state = true;

    if (read_client_data(&input_client_data, client_input_flags)){
        uint32_t gpio_index = input_client_data.client_state->
                              devices[input_client_data.device_index - 1].
                              gpio_number;
    
        server_set_device_state_and_update_flash(active_uart_server_connections[input_client_data.client_index - 1].pin_pair,
            active_uart_server_connections[input_client_data.client_index - 1].uart_instance,
            gpio_index,
            input_client_data.device_state,
            input_client_data.flash_client_index);
    
        printf("\nDevice[%u] %s.\n", input_client_data.device_index, input_client_data.device_state == 1 ? "ON" : "OFF");
    }
}

/**
 * @brief Prints all currently active UART connections to the console.
 *
 * Displays each valid UART connection with its associated TX/RX pins and UART instance number.
 */
static inline void display_active_clients(void){    
    printf("\nThese are the active client connections:\n");
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
            save_running_state();
            break;
        case 5:
            build_preset_configuration();
            break;
        case 6:
            load_configuration();
            break;
        case 7:
            reset_configuration();
            break;
        case 8:
            clear_screen();
            break;
        case 9:
            restart_application();
            break;
        default:
            printf("Out of range. Try again.\n");
            break;
        }
    }
    
/**
 * @brief Reads a menu selection from the user and validates it.
 *
 * This function displays the available menu options and waits for the user 
 * to select a valid one. If the selected option is 0, the function exits early.
 * Otherwise, it loops until a valid input is received.
 *
 * @param[out] menu_option Pointer to the variable where the selected menu option will be stored.
 */
static void read_menu_option(uint32_t *menu_option){
    bool correct_menu_option_input = false;
    while (!correct_menu_option_input){
        display_menu_options();
        if (choose_menu_option(menu_option)){
            if (*menu_option == 0){
                return;
            }else{
                correct_menu_option_input = true;
            }
        }else{
            print_input_error();
            printf("\n");
        }
    }
}

/**
 * @brief Entry point for the USB menu system.
 *
 * Prints a welcome screen on first call, then shows the menu options.
 * Processes user selection via `select_action(menu_option)`.
 */
void server_display_menu(void){
    if (first_display){ 
        first_display = false;
        print_delimitor();
        printf("Welcome!");
        display_active_clients();
        printf("\n");
    }

    uint32_t menu_option;
    read_menu_option(&menu_option);
    select_action(menu_option);

    print_delimitor();
}
