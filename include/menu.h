/**
 * @file menu.h
 * @brief User interface menu functions for the UART server.
 *
 * Provides functions to interact with the server through a USB CLI interface.
 * Includes logic to display menus and handle user commands.
 */

#ifndef MENU_H
#define MENU_H

#ifndef BUFFER_MAX_STRING_SIZE
#define BUFFER_MAX_STRING_SIZE 65
#endif

#ifndef BUFFER_MAX_NUMBER_OF_STRINGS
#define BUFFER_MAX_NUMBER_OF_STRINGS 50
#endif

#ifndef PERIODIC_CONSOLE_CHECK_TIME_MS
#define PERIODIC_CONSOLE_CHECK_TIME_MS 1500
#endif

#ifndef DUMP_BUFFER_WAKEUP_MESSAGE
#define DUMP_BUFFER_WAKEUP_MESSAGE 0xBFFEBFFE
#endif

#ifndef BLINK_LED_WAKEUP_MESSAGE
#define BLINK_LED_WAKEUP_MESSAGE 0xEDEDEDED
#endif

extern volatile char reconnection_buffer[BUFFER_MAX_NUMBER_OF_STRINGS][BUFFER_MAX_STRING_SIZE];
extern volatile uint32_t reconnection_buffer_len;
extern volatile uint32_t reconnection_buffer_index;

/**
 * @brief Displays the UART server's command-line interface menu.
 *
 * This function handles interaction with the user over USB stdio. It prints
 * options, receives commands, and updates the server state accordingly.
 * It is typically called in a loop after UART client connections are established.
 */
void server_display_menu(void);

void periodic_wakeup();

void printf_and_update_buffer(const char *string);

inline void print_cancel_message(void){
    printf_and_update_buffer("0. cancel\n");
}

inline void print_input_error(void){
    printf_and_update_buffer("Invalid input or overflow. Try again.\n");
}

inline void print_delimitor(void){
    printf_and_update_buffer("\n****************************************************\n\n");
}

#endif
