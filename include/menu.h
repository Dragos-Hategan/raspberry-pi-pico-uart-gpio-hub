/**
 * @file menu.h
 * @brief User interface menu functions for the UART server.
 *
 * Provides functions to interact with the server through a USB CLI interface.
 * Includes logic to display menus and handle user commands.
 */

#ifndef MENU_H
#define MENU_H

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

#endif
