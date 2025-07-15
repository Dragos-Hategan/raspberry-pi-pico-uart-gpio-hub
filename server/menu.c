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

static inline void print_input_error(void){
    printf("Invalid input or overflow. Try again.\n");
}

static inline void print_delimitor(void){
    printf("\n****************************************************\n\n");
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
static void read_device_state(uint32_t *device_state){
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
        if (choose_device(device_index, &flash_state->clients[flash_client_index])){
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

/**
 * @brief Repeatedly prompts the user to select a valid preset configuration index.
 *
 * - Displays the list of available preset configuration slots.
 * - Loops until a valid selection is made via `choose_flash_configuration_index()`.
 * - Stores the final selection in `flash_configuration_index`.
 *
 * @param flash_configuration_index Output pointer for selected configuration index (1-based from user input).
 * @param flash_client_index Index of the client whose configurations are being listed.
 */
static void read_flash_configuration_index(uint32_t *flash_configuration_index, uint32_t flash_client_index){
    bool correct_flash_configuration_input = false;
    while (!correct_flash_configuration_input){
        for (uint32_t configuration_index = 1; configuration_index <= NUMBER_OF_POSSIBLE_PRESETS; configuration_index++){
            printf("%u. Preset Config[%u]\n", configuration_index, configuration_index);
        }
        if (choose_flash_configuration_index(flash_configuration_index, flash_client_index)){
            correct_flash_configuration_input = true;
        }else{
            print_input_error();
            printf("\n");
        }
    }
}

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
    read_flash_configuration_index(&flash_configuration_index, flash_client_index);    
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
 * @brief Reads a reset variant option from the user.
 *
 * Prompts the user with a menu to choose what part of the client should be reset.
 * Keeps asking until a valid input is provided or the user cancels with 0.
 *
 * @param reset_variant Pointer to store the selected option.
 *                      Valid values:
 *                      - 0: Cancel
 *                      - 1: Reset running configuration
 *                      - 2: Reset preset configurations
 *                      - 3: Reset all client data
 */
static void read_reset_variant(uint32_t *reset_variant){
    bool correct_reset_variant_input = false;
    while (!correct_reset_variant_input){
        if (choose_reset_variant(reset_variant)){
            if (*reset_variant == 0){
                return;
            }else{
                correct_reset_variant_input = true;
            }
        }else{
            print_input_error();
            printf("\n");
        }
    }
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
    uint32_t client_index;
    read_client_index(&client_index);
    if (!client_index){
        return;
    }

    uint32_t flash_client_index;
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    find_corect_client_index_from_flash(&flash_client_index, client_index, flash_state);
    const client_t* client = (const client_t *)&flash_state->clients[flash_client_index];

    printf("\n");
    server_print_running_client_state(client);
    printf("\n");
    server_print_client_preset_configurations(client);

    uint32_t reset_choice;
    read_reset_variant(&reset_choice);
    if (!reset_choice){
        return;
    }

    if (reset_choice == 1){
        reset_running_configuration(flash_client_index);
    }else if (reset_choice == 2){
        reset_preset_configuration(flash_client_index);
    }else{
        reset_all_client_data(flash_client_index);
    }
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
    printf("\nConfiguration Preset[%u] loaded!\n", flash_configuration_index + 1);
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
    uint32_t client_index;
    read_client_index(&client_index);
    if (!client_index){
        return;
    }

    uint32_t flash_client_index;
    const server_persistent_state_t *flash_state = (const server_persistent_state_t *)SERVER_FLASH_ADDR;
    find_corect_client_index_from_flash(&flash_client_index, client_index, flash_state);
    const client_t* client = (const client_t *)&flash_state->clients[flash_client_index];

    printf("\n");
    server_print_running_client_state(client);
    printf("\n");
    server_print_client_preset_configurations(client);

    uint32_t flash_configuration_index;
    read_flash_configuration_index(&flash_configuration_index, flash_client_index);    
    if (!flash_configuration_index){
        return;
    }

    if (flash_configuration_index == 0){
        return;
    }else{
        load_configuration_into_running_state(flash_configuration_index - 1, flash_client_index);
    }
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
    printf("\nConfiguration saved in Preset[%u]!\n", flash_configuration_index + 1);
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
static void save_running_configuration(uint32_t flash_client_index, const client_t* client){
    printf("\n");

    server_print_client_preset_configurations(client);

    uint32_t flash_configuration_index;
    read_flash_configuration_index(&flash_configuration_index, flash_client_index);    
    if (!flash_configuration_index){
        return;
    }

    save_running_configuration_into_preset_configuration(flash_configuration_index - 1, flash_client_index);
}

static void set_configuration_devices(uint32_t flash_client_index, uint32_t flash_configuration_index){
    // afisez starea, cer input de device, cer 1. ON, 2. 0FF, 3. TOGGLE
    
}

static void build_configuration(uint32_t flash_client_index, const client_t* client){
    printf("\n");
    server_print_client_preset_configurations(client);
    
    uint32_t flash_configuration_index;
    read_flash_configuration_index(&flash_configuration_index, flash_client_index);    
    if (!flash_configuration_index){
        return;
    }
    flash_configuration_index--;

    set_configuration_devices(flash_client_index, flash_configuration_index);

    printf("Building Configuration Complete.\n");
}

/**
 * @brief Repeatedly prompts the user to select a valid saving option.
 *
 * - Displays the saving options (running or built).
 * - Repeats until a valid non-zero option is selected via `choose_saving_option()`.
 *
 * @param saving_option Output pointer to store the selected saving option.
 */
static void read_saving_option(uint32_t *saving_option){
    bool correct_saving_option_input = false;
    while (!correct_saving_option_input){
        printf("\n1. Save running configuration into preset.\n2. Build and save preset configuration.\n");
        if (choose_saving_option(saving_option)){
            if (*saving_option == 0){
                return;
            }else{
                correct_saving_option_input = true;
            }
        }else{
            print_input_error();
        }
    }
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
    
    printf("\n");
    server_print_running_client_state(client);

    uint32_t saving_option;
    read_saving_option(&saving_option);
    if (!saving_option){
        return;
    }

    if (saving_option == 1){
        save_running_configuration(flash_client_index, client);
    }else{
        build_configuration(flash_client_index, client);
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

    uint32_t device_state;
    read_device_state(&device_state);
    if (!device_state){
        return;
    }
    device_state %= 2;

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
            save_configuration();
            break;
        case 5:
            load_configuration();
            break;
        case 6:
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
        "4. Save configuration\n"
        "5. Load configuration\n"
        "6. Reset configuration\n"
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
