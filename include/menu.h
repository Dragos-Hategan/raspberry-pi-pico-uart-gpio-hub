/**
 * @file menu.h
 * @brief User interface menu functions for the UART server.
 *
 * Provides functions to interact with the server through a USB CLI interface.
 * Includes logic to display menus and handle user commands.
 */

#ifndef MENU_H
#define MENU_H

extern volatile char reconnection_buffer[10][10];
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

inline void print_cancel_message(void){
    printf("0. cancel\n");
}

inline void print_input_error(void){
    printf("Invalid input or overflow. Try again.\n");
}

inline void print_delimitor(void){
    printf("\n****************************************************\n\n");
}

inline void clear_screen(){
    printf("\033[2J");    // delete screen
    printf("\033[H");     // move cursor to upper left screen
}

inline void display_menu_options(){
    printf(
        "Options:\n"
        "1. Display Clients\n"
        "2. Set Client's Device\n"
        "3. Toggle Client's Device\n"
        "4. Save Running State Into Preset Configuration\n"
        "5. Build And Save Preset Configuration\n"
        "6. Load Preset Configuration Into Running State\n"
        "7. Reset Configuration\n"
        "8. Clear Screen\n"
        "9. Restart System\n"
    );
}

void print_and_update_buffer(char *string){
    printf(string);
    
}

#endif
