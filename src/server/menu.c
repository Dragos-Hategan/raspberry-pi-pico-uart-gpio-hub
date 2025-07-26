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

#include "pico/multicore.h"
#include "pico/time.h"
#include "pico/stdio_usb.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"

#include "functions.h"
#include "input.h"
#include "server.h"
#include "menu.h"

static bool first_display = true;
static volatile bool console_connected = false;
static volatile bool console_disconnected = false;
static repeating_timer_t repeating_timer;
volatile char reconnection_buffer[BUFFER_MAX_NUMBER_OF_STRINGS][BUFFER_MAX_STRING_SIZE] = {0};
volatile uint32_t reconnection_buffer_len = 0;
volatile uint32_t reconnection_buffer_index = 0;

void server_display_menu(void);

/**
 * @brief Prints a string and stores it into the reconnection buffer.
 *
 * Maintains a ring-like buffer of the last printed strings (max BUFFER_MAX_NUMBER_OF_STRINGS).
 * If the buffer is full, it shifts all entries up to make room for the new one.
 *
 * @param string The string to print and store.
 */
void print_and_update_buffer(const char *string){
    printf("%s", string);

    if (BUFFER_MAX_NUMBER_OF_STRINGS - 1 == reconnection_buffer_index){
        for (uint8_t buffer_array_index = 0; buffer_array_index < reconnection_buffer_index - 1; buffer_array_index++){
            memcpy( (char *)reconnection_buffer[buffer_array_index],
                    (const char *)reconnection_buffer[buffer_array_index + 1],
                    BUFFER_MAX_STRING_SIZE
            );
        }
        memcpy( (char *)reconnection_buffer[reconnection_buffer_index - 1],
                (const char *)string,
                strlen(string) + 1
            );
    }else if (BUFFER_MAX_NUMBER_OF_STRINGS - 1 > reconnection_buffer_index){
        memcpy( (char *)reconnection_buffer[reconnection_buffer_index],
                (const char *)string,
                strlen(string) + 1
            );
        reconnection_buffer_index++;
    }
}

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

    print_and_update_buffer("\nBuilding Configuration Complete.\n");
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
    
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "\nDevice[%u] Toggled.\n", input_client_data.device_index);
        print_and_update_buffer(string);
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
    
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "\nDevice[%u] %s.\n",
            input_client_data.device_index,
            input_client_data.device_state == 1 ? "ON" : "OFF");
        print_and_update_buffer(string);
    }
}

/**
 * @brief Prints all currently active UART connections to the console.
 *
 * Displays each valid UART connection with its associated TX/RX pins and UART instance number.
 */
static inline void display_active_clients(void){    
    print_and_update_buffer("\nThese are the active client connections:\n");
    for (uint8_t index = 1; index <= active_server_connections_number; index++){
        char string[BUFFER_MAX_STRING_SIZE];
        snprintf(string, sizeof(string), "%u. GPIO Pin Pair=[%u,%u]. UART Instance=uart%d.\n", index, 
            active_uart_server_connections[index - 1].pin_pair.tx,
            active_uart_server_connections[index - 1].pin_pair.rx,
            UART_NUM(active_uart_server_connections[index - 1].uart_instance));
        print_and_update_buffer(string);
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
            print_and_update_buffer("Out of range. Try again.\n");
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
            print_and_update_buffer("\n");
        }
    }
}

/**
 * @brief Periodically checks the USB console connection state.
 *
 * This function is called by a repeating timer on core1. It detects USB terminal
 * disconnection/reconnection and triggers an inter-core message to reprint
 * the buffered output.
 *
 * @param repeating_timer Pointer to the timer structure (unused).
 * @return Always returns true to keep the repeating timer active.
 */
static bool check_console_state(repeating_timer_t *repeating_timer){
    if (console_connected && !stdio_usb_connected()){
        console_connected = false;
        console_disconnected = true;
    }else if (console_disconnected && stdio_usb_connected()){
        console_connected = true;
        console_disconnected = false;
        multicore_fifo_push_blocking(DUMP_BUFFER_WAKEUP_MESSAGE);
    }
    return true;
}

/**
 * @brief Sets up a repeating timer to monitor USB console connectivity.
 *
 * Initializes the `console_connected` flag and starts a repeating timer that
 * calls `check_console_state()` every `PERIODIC_CONSOLE_CHECK_TIME_MS` milliseconds
 * to detect USB terminal reconnections.
 */
static void setup_repeating_timer_for_console_activity(){
    console_connected = true;
    add_repeating_timer_ms(PERIODIC_CONSOLE_CHECK_TIME_MS, check_console_state, NULL, &repeating_timer);
}

/**
 * @brief Core1 entry point that waits for print commands and dumps the buffer.
 *
 * Blocks using `__wfe()` until it receives an inter-core message via FIFO.
 * When triggered, it prints the contents of `reconnection_buffer` with a short delay.
 */
void periodic_wakeup(){
    while (true) {
        __wfe();
        uint32_t cmd = multicore_fifo_pop_blocking();
        if (cmd == DUMP_BUFFER_WAKEUP_MESSAGE) {
            for (uint8_t index = 0; index < reconnection_buffer_index; index++) {
                printf("%s", reconnection_buffer[index]);
                sleep_ms(2); 
            }
        }
        if (cmd == BLINK_LED_WAKEUP_MESSAGE){
            fast_blink_onboard_led();
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
    setup_repeating_timer_for_console_activity();
    
    if (first_display){ 
        first_display = false;
        print_delimitor();
        print_and_update_buffer("Welcome!");
        display_active_clients();
        print_and_update_buffer("\n");
    }

    uint32_t menu_option;
    read_menu_option(&menu_option);
    select_action(menu_option);

    print_delimitor();
}
