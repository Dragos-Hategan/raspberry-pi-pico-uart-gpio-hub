/**
 * @file input.h
 * @brief Interface for reading validated numeric input from the user.
 *
 * This header provides a function that prompts the user for a numeric value
 * and validates whether the input falls within a specified range. Useful in
 * menu-based CLI applications where user input must be constrained.
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

#include "types.h"

#ifndef MINIMUM_MENU_OPTION_INDEX_INPUT
#define MINIMUM_MENU_OPTION_INDEX_INPUT 1
#endif

#ifndef MAXIMUM_MENU_OPTION_INDEX_INPUT
#define MAXIMUM_MENU_OPTION_INDEX_INPUT 8
#endif

#ifndef MINIMUM_SAVING_OPTION_INPUT 
#define MINIMUM_SAVING_OPTION_INPUT 0
#endif

#ifndef MAXIMUM_SAVING_OPTION_INPUT 
#define MAXIMUM_SAVING_OPTION_INPUT 2
#endif

#ifndef MINIMUM_FLASH_CONFIGURATION_INDEX_INPUT 
#define MINIMUM_FLASH_CONFIGURATION_INDEX_INPUT 0
#endif

#ifndef MAXIMUM_FLASH_CONFIGURATION_INDEX_INPUT 
#define MAXIMUM_FLASH_CONFIGURATION_INDEX_INPUT NUMBER_OF_POSSIBLE_PRESETS
#endif

#ifndef MINIMUM_CLIENT_INDEX_INPUT 
#define MINIMUM_CLIENT_INDEX_INPUT 0
#endif

#ifndef MINIMUM_DEVICE_INDEX_INPUT 
#define MINIMUM_DEVICE_INDEX_INPUT 0
#endif

#ifndef MAXIMUM_DEVICE_INDEX_INPUT 
#define MAXIMUM_DEVICE_INDEX_INPUT MAX_NUMBER_OF_GPIOS
#endif

#ifndef MINIMUM_DEVICE_STATE_INPUT 
#define MINIMUM_DEVICE_STATE_INPUT 0
#endif

#ifndef MAXIMUM_DEVICE_STATE_INPUT 
#define MAXIMUM_DEVICE_STATE_INPUT 2
#endif

#ifndef MINIMUM_RESET_VARIANT_INPUT 
#define MINIMUM_RESET_VARIANT_INPUT 0
#endif

#ifndef MAXIMUM_RESET_VARIANT_INPUT 
#define MAXIMUM_RESET_VARIANT_INPUT 3
#endif

/**
 * @brief Prompts the user to enter a number within a specified range.
 *        Displays a message, reads the input, and validates that it's within [min, max].
 *
 * @param message       The message to display as a prompt
 * @param out           Pointer where the resulting number will be stored
 * @param min           Minimum allowed value (inclusive)
 * @param max           Maximum allowed value (inclusive)
 * @return true if the input is valid and within range, false otherwise
 */
bool read_user_choice_in_range(const char* message, uint32_t* out, uint32_t min, uint32_t max);

/**
 * @brief Prompts the user to choose a reset type.
 *
 * - Displays the available reset options:
 *     1. Running state
 *     2. Preset configurations
 *     3. All client data
 * - Uses range-validated input to ensure a valid selection.
 *
 * @param reset_variant Pointer to the output variable for the selected option.
 * @return true if a valid option was selected, false otherwise.
 */
bool choose_reset_variant(uint32_t *reset_variant);

/**
 * @brief Prompts the user to choose which connected client to access.
 *
 * @param client_index Output pointer for selected index (1-based).
 * @return true if valid client selected, false otherwise.
 */
bool choose_client(uint32_t *client_index);

/**
 * @brief Prompts the user to select a GPIO device to modify.
 *
 * @param device_index Output pointer for selected index (1-based).
 * @param client_state The state structure of the selected client.
 * @return true if valid device selected, false otherwise.
 */
bool choose_device(uint32_t *device_index, const client_state_t *client_state);

/**
 * @brief Prompts the user to enter a desired state: ON (1) or OFF (0).
 *
 * @param device_state Pointer to store the selected boolean state.
 * @return true if valid input received, false otherwise.
 */
bool choose_state(uint32_t *device_state);

/**
 * @brief Prompts the user to select a preset configuration index.
 *
 * Displays a cancel option and asks the user to choose a preset index within valid bounds.
 *
 * @param flash_configuration_index Output pointer to store the selected preset index.
 * @return true if valid input received, false otherwise.
 */
bool choose_flash_configuration_index(uint32_t *flash_configuration_index);

/**
 * @brief Prompts the user to select a menu option from the main CLI.
 *
 * This function displays a message prompting the user to pick a number
 * between 1 and 8, representing the available menu options.
 * It reads and validates the input, and stores the selected option in `menu_option`.
 *
 * The valid range is:
 * - 1: Display clients
 * - 2: Set client's device
 * - 3: Toggle client's device
 * - 4: Save running state into preset configuration
 * - 5: Build and save preset configuration
 * - 6: Load preset configuration into running state
 * - 7: Reset configuration
 * - 8: Clear Screen
 *
 * @param[out] menu_option Pointer to store the selected menu option.
 * @return true if a valid input was received, false otherwise.
 */
bool choose_menu_option(uint32_t *menu_option);

/**
 * @brief Reads a valid device index from the user for a given client.
 *
 * This function loops until the user provides a valid device index 
 * (or 0 to cancel). It checks if the selected index is valid for 
 * the given client using the server's persistent state and the client's current state.
 *
 * @param[out] device_index Pointer to the variable where the selected device index will be stored.
 * @param[in] flash_client_index Index of the client in the flash-stored client list.
 * @param[in] flash_state Pointer to the full persistent server state structure.
 * @param[in] client_state Pointer to the current runtime state of the active client.
 */
void read_device_index(uint32_t *device_index, uint32_t flash_client_index, const server_persistent_state_t *flash_state, const client_state_t *client_state);

/**
 * @brief Repeatedly prompts the user to select a valid preset configuration index.
 *
 * - Displays the list of available preset configuration slots.
 * - Loops until a valid selection is made via `choose_flash_configuration_index()`.
 * - Stores the final selection in `flash_configuration_index`.
 *
 * @param flash_configuration_index Output pointer for selected configuration index (1-based from user input).
 */
void read_flash_configuration_index(uint32_t *flash_configuration_index);

/**
 * @brief Reads a valid ON/OFF state input from the user.
 *
 * Prompts until a valid binary state is selected (0 or 1).
 *
 * @param device_state Output pointer to store the selected state (true = ON, false = OFF).
 */
void read_device_state(uint32_t *device_state);

/**
 * @brief Prompts the user to select a client index.
 *
 * Repeats until a valid client is selected or cancel (0) is entered.
 *
 * @param client_index Output pointer to store the selected client index (1-based).
 */
void read_client_index(uint32_t *client_index);

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
void read_reset_variant(uint32_t *reset_variant);

/**
 * @brief Collects validated input from the user for client-related operations.
 *
 * This function performs multiple user interactions based on the provided `client_input_flags_t`.
 * It reads values such as client index, device index, GPIO state, preset configuration index, or
 * reset choice, and stores them into a `input_client_data_t` structure.
 *
 * If `build_preset` is true, the function also calls `set_configuration_devices()` automatically.
 * 
 * This function short-circuits (returns early) if any step is canceled or receives invalid input.
 *
 * @param[out] client_data Pointer to the structure where the input values will be stored.
 * @param[in] client_input_flags Specifies which inputs are required (see `client_input_flags_t`).
 */
bool read_client_data(input_client_data_t *client_data, client_input_flags_t client_input_flags);

#endif 
