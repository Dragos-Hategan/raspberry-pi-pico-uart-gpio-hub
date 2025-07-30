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

    sleep_ms(1000);

    server_load_running_states_to_active_clients();
    blink_onboard_led();
}

/**
 * @brief Final initialization stage and entry into USB CLI display loop.
 *
 * Starts the LED timer, launches core1, and enters a loop
 * waiting for USB CLI connections to launch the user interface.
 */
static void last_inits_and_display_launch(){
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
