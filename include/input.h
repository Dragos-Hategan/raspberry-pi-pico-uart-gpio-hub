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

#endif 
