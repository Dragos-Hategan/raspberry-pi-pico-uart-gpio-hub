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
#define PERIODIC_CONSOLE_CHECK_TIME_MS 2000
#endif

#ifndef PERIODIC_ONBOARD_LED_BLINK_TIME_MS
#define PERIODIC_ONBOARD_LED_BLINK_TIME_MS 2500
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

void print_and_update_buffer(const char *string);

inline void print_cancel_message(void){
    print_and_update_buffer("0. cancel\n");
}

inline void print_input_error(void){
    print_and_update_buffer("Invalid input or overflow. Try again.\n");
}

inline void print_delimitor(void){
    print_and_update_buffer("\n****************************************************\n\n");
}

inline void clear_screen(){
    print_and_update_buffer("\033[2J");    // delete screen
    print_and_update_buffer("\033[H");     // move cursor to upper left screen
}

inline void display_menu_options(){
    print_and_update_buffer("Options:\n");
    print_and_update_buffer("1. Display Clients\n");
    print_and_update_buffer("2. Set Client's Device\n");
    print_and_update_buffer("3. Toggle Client's Device\n");
    print_and_update_buffer("4. Save Running State Into Preset Configuration\n");
    print_and_update_buffer("5. Build And Save Preset Configuration\n");
    print_and_update_buffer("6. Load Preset Configuration Into Running State\n");
    print_and_update_buffer("7. Reset Configuration\n");
    print_and_update_buffer("8. Clear Screen\n");
    print_and_update_buffer("9. Restart System\n");
}

#endif
