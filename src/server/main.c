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

#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "hardware/irq.h"
#include "hardware/regs/usb.h"
#include "hardware/structs/usb.h"

#include "functions.h"
#include "server.h"
#include "menu.h"

static repeating_timer_t repeating_timer;
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
 * @brief Repeating timer callback to trigger onboard LED blink on core1.
 *
 * Pushes an inter-core message to wake up core1 and trigger LED blinking.
 *
 * @param repeating_timer Unused.
 * @return Always true to keep the timer running.
 */
static bool short_onboard_led_blink(repeating_timer_t *repeating_timer){
    multicore_fifo_push_blocking(BLINK_LED_WAKEUP_MESSAGE);
    return true;
}

/**
 * @brief Initializes a repeating timer for periodic onboard LED blinking.
 *
 * Every `PERIODIC_ONBOARD_LED_BLINK_TIME_MS`, a wakeup message is sent
 * to core1 for triggering a short LED blink.
 */
static void setup_repeating_timer_for_periodic_onboard_led_blink(){
    add_repeating_timer_ms(PERIODIC_ONBOARD_LED_BLINK_TIME_MS, short_onboard_led_blink, NULL, &repeating_timer);
}

/**
 * @brief Detects UART clients and loads their saved GPIO states.
 *
 * Loops until at least one UART connection is detected. After that:
 * - Loads the last saved GPIO states for each client.
 * - Performs a confirmation blink.
 */
static void find_clients(void){
    while(!server_find_connections()) tight_loop_contents();

    server_load_running_states_to_active_clients();
    blink_onboard_led();
}

/**
 * @brief Final initialization stage and entry into USB CLI display loop.
 *
 * Sets up USB IRQs, starts the LED timer, launches core1, and enters a loop
 * waiting for USB CLI connections to launch the user interface.
 */
static void last_inits_and_display_launch(){
    #if RESTART_SYSTEM_AT_USB_RECONNECTION
        setup_usb_irq();
    #endif

    #if PERIODIC_ONBOARD_LED_BLINK_SERVER || PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS
        setup_repeating_timer_for_periodic_onboard_led_blink();
    #endif

    multicore_launch_core1(periodic_wakeup);

    while(true){
        if (stdio_usb_connected()){
            server_display_menu();
        }
    }
}

/**
 * @brief Initializes LED and USB, detects clients, and enters UI loop.
 */
static void entry_point(){
    init_onboard_led_and_usb();
    find_clients();
    last_inits_and_display_launch();
}

/**
 * @brief Main entry point of the UART server application.
 *
 * If the system rebooted due to a watchdog reset, applies a short delay
 * before continuing initialization. Delegates logic to `entry_point()`.
 *
 * @return Exit code (not used).
 */
int main(void){
    if (watchdog_caused_reboot()){
        sleep_ms(100);
        entry_point();
    }else{
        entry_point();
    }
}
