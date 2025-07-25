/**
 * @file main.c
 * @brief UART server application for Raspberry Pi Pico.
 *
 * This program runs on a Raspberry Pi Pico acting as a central UART server.
 * It scans through all possible UART0 and UART1 TX/RX pin pairs to detect
 * connection requests from clients. When a handshake is successfully completed,
 * the server stores the connection in an internal list.
 *
 * It then loads previously saved GPIO states for those clients and exposes
 * a USB CLI interface for real-time control.
 */

#include <stdbool.h>

#include "hardware/watchdog.h"
#include "hardware/irq.h"
#include "hardware/regs/usb.h"
#include "hardware/structs/usb.h"

#include "functions.h"
#include "server.h"
#include "menu.h"

volatile bool usb_connected = false;
volatile bool usb_disconected = false;

/**
 * @brief USB interrupt handler for VBUS connection detection.
 *
 * Monitors the USB connection status using `usb_hw->sie_status` flags.
 * - Sets `usb_connected` when USB becomes connected.
 * - Sets `usb_disconected` when USB is disconnected.
 * - If a disconnection followed by a reconnection is detected,
 *   triggers a reset sequence for all clients and enables watchdog reset.
 */
static void usb_irq_handler(void) {
    if (!usb_connected && (usb_hw->sie_status & USB_SIE_STATUS_CONNECTED_BITS)){
        usb_connected = true;
    }
    if (usb_connected && !(usb_hw->sie_status & USB_SIE_STATUS_CONNECTED_BITS)){
        usb_connected = false;
        usb_disconected = true;
    }
    if (usb_disconected && (usb_hw->sie_status & USB_SIE_STATUS_CONNECTED_BITS)){
        signal_reset_for_all_clients();
        watchdog_reboot(0, 0, 0);
        while (true) tight_loop_contents();
    }

}

/**
 * @brief Configures USB interrupt handling for VBUS detection.
 *
 * Enables the VBUS detect interrupt and attaches `usb_irq_handler`
 * to the USB controller IRQ with default priority.
 */
static void setup_usb_irq(void) {
    hw_set_bits(&usb_hw->inte, USB_INTE_VBUS_DETECT_BITS);
    irq_add_shared_handler(USBCTRL_IRQ, usb_irq_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    if (!irq_is_enabled(USBCTRL_IRQ)){
        irq_set_enabled(USBCTRL_IRQ, true);
    }
}

/**
 * @brief Attempts to discover all active UART clients.
 *
 * Continuously calls `server_find_connections()` until at least
 * one connection is established. Then:
 * - Blinks onboard LED.
 * - Loads saved GPIO states for clients.
 * - Installs USB IRQ handler.
 * - Enters USB CLI loop via `server_display_menu()`.
 */
static void find_clients(void){
    while(!server_find_connections()) tight_loop_contents();

    blink_onboard_led();
    server_load_running_states_to_active_clients();

    setup_usb_irq();

    while(true){
        if (stdio_usb_connected()){
            server_display_menu();
        }
    }
}

/**
 * @brief Main entry point of the UART server application.
 *
 * Steps:
 * - Initializes USB stdio and onboard LED.
 * - Scans for valid client UART connections via `server_find_connections()`.
 * - Once at least one connection is established:
 *   - Blinks the onboard LED.
 *   - Loads previous GPIO states from internal flash.
 * - Enters a USB CLI loop (if connected).
 *
 * @return int Exit code (not used).
 */
int main(void){
    if (watchdog_caused_reboot()){
        sleep_ms(100);
        init_led_and_usb();
        find_clients();
    }else{
        init_led_and_usb();
        find_clients();
    }
}
