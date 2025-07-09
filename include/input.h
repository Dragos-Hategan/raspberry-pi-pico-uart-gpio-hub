#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif

#endif // INPUT_H
