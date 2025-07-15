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
#include "menu.h"

bool first_display = true;

void server_display_menu(void);

/**
 * @brief Resets the currently active (running) configuration of a client.
 *
 * - Loads the full persistent server state from flash.
 * - Resets the `running_client_state` using `server_reset_configuration()`.
 * - Sends the updated state to the client over UART.
 * - Saves the modified state back to flash.
 *
 * @param flash_client_index Index of the client in the persistent flash structure.
 */
static void reset_running_configuration(uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    server_reset_configuration(&state.clients[flash_client_index].running_client_state);

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);

    save_server_state(&state);

    printf("\nRunning Configuration Reset.\n");
}

/**
 * @brief Resets one of the preset configurations for a given client.
 *
 * - Loads the full persistent server state from flash.
 * - Prompts the user to select a valid preset index.
 * - Resets the selected preset using `server_reset_configuration()`.
 * - Saves the modified state back to flash.
 *
 * @param flash_client_index Index of the client in the persistent flash structure.
 */
static void reset_preset_configuration(uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    uint32_t flash_configuration_index;
    printf("\n");
    read_flash_configuration_index(&flash_configuration_index);    
    if (!flash_configuration_index){
        return;
    }

    server_reset_configuration(&state.clients[flash_client_index].preset_configs[flash_configuration_index - 1]);

    save_server_state(&state);

    printf("\nPreset Configuration [%u] Reset.\n", flash_configuration_index);
}

/**
 * @brief Resets all data associated with a client.
 *
 * - Resets the running state of the client and sends it over UART.
 * - Resets all preset configurations.
 * - Saves the updated state back to flash.
 *
 * @param flash_client_index Index of the client in the persistent flash structure.
 */
static void reset_all_client_data(uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);
    
    server_reset_configuration(&state.clients[flash_client_index].running_client_state);

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);

    for (uint8_t configuration_index = 0; configuration_index < NUMBER_OF_POSSIBLE_PRESETS; configuration_index++){
        server_reset_configuration(&state.clients[flash_client_index].preset_configs[configuration_index]);
    }

    save_server_state(&state);

    printf("\nAll Client Data Reset.\n");
}

/**
 * @brief Loads a preset configuration into a client's running state and applies it via UART.
 *
 * - Loads the full persistent state from flash.
 * - Copies the selected preset configuration into the running configuration.
 * - Sends the new configuration to the client via UART.
 * - Saves the updated state back to flash.
 *
 * @param flash_configuration_index Index of the preset configuration to load (0-based).
 * @param flash_client_index Index of the client in the flash-stored structure.
 */
static void load_configuration_into_running_state(uint32_t flash_configuration_index, uint32_t flash_client_index){
    server_persistent_state_t state;
    load_server_state(&state);

    memcpy(
        &state.clients[flash_client_index].running_client_state,
        &state.clients[flash_client_index].preset_configs[flash_configuration_index],
        sizeof(client_state_t)
    );

    server_send_client_state(state.clients[flash_client_index].uart_connection.pin_pair,
                            state.clients[flash_client_index].uart_connection.uart_instance,
                            &state.clients[flash_client_index].running_client_state);
    save_server_state(&state);
    printf("\nConfiguration Preset[%u] Loaded!\n", flash_configuration_index + 1);
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
    printf("\nConfiguration saved in Preset[%u].\n", flash_configuration_index + 1);
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
            reset_preset_configuration(input_client_data.flash_client_index);
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
    
        printf("\nDevice[%u] Toggled.\n");
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
        default:
            printf("Out of range. Try again.\n");
            break;
        }
    }
    
static inline void display_menu_options(){
    printf(
        "Options:\n"
        "1. Display clients\n"
        "2. Set client's device\n"
        "3. Toggle client's device\n"
        "4. Save running state into preset configuration\n"
        "5. Build and save preset configuration\n"
        "6. Load preset configuration into running state\n"
        "7. Reset configuration\n"
    );
}
    
/**
 * @brief Reads a menu selection from the user and processes it.
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

static inline void clear_screen(){
    printf("\033[2J");    // delete screen
    printf("\033[H");     // move cursor to upper left screen
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
        clear_screen();
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
