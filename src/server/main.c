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

#include "server.h"
#include "functions.h"
#include "menu.h"

static repeating_timer_t repeating_timer;
spin_lock_t *uart_lock = NULL;

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

void periodic_wakeup(void){
    multicore_fifo_drain();
    while (true) {
        __wfe();
        if (multicore_fifo_rvalid()) {
            uint32_t cmd = multicore_fifo_pop_blocking();

            if (cmd == DUMP_BUFFER_WAKEUP_MESSAGE) {
                for (uint8_t index = 0; index < reconnection_buffer_index; index++) {
                    printf("%s", reconnection_buffer[index]);
                    sleep_ms(2); 
                }
            } 
            else if (cmd == BLINK_LED_WAKEUP_MESSAGE){
                #if PERIODIC_ONBOARD_LED_BLINK_SERVER
                    fast_blink_onboard_led();
                #endif
                
                #if PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS
                    send_fast_blink_onboard_led_to_clients();
                #endif
            }
        }
    }
}

/**
 * @brief Detects UART clients and loads their saved GPIO states.
 *
 * Loops until at least one UART connection is detected. After that:
 * - Performs a confirmation blink.
 * - Loads the last saved GPIO states for each client.
 */
static void find_clients(void){
    while(!server_find_connections()) tight_loop_contents();

    blink_onboard_led_blocking();
    
    server_load_running_states_to_active_clients();
}

/**
 * @brief Configures all RX pins of active server UART connections as outputs set to LOW.
 *
 * This is used before entering low-power modes where these pins
 * are repurposed (to trigger wakeup signals).
 *
 * The function:
 * - Deinitializes each RX pin (removing previous uart)
 * - Re-initializes it as a standard GPIO
 * - Sets it as output and drives it LOW
 */
static void set_pins_as_output_for_dormant_wakeup(){
    for (uint8_t connection_index = 0; connection_index < active_server_connections_number; connection_index++){
        uint8_t pin = active_uart_server_connections[connection_index].pin_pair.rx;
        gpio_deinit(pin);
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, 0);
    }
}

/**
 * @brief Final initialization stage and entry into USB CLI display loop.
 *
 * Performs the last setup steps before the main server loop:
 * - Sets RX pins as GPIO outputs for wakeup handling
 * - Starts a periodic onboard LED blink timer (if enabled)
 * - Launches core 1 to handle periodic wakeup tasks
 * - Waits for a USB CLI connection and launches the server menu UI
 */
static void last_inits_and_display_launch(){        
    #if PERIODIC_ONBOARD_LED_BLINK_SERVER || PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS
        setup_repeating_timer_for_periodic_onboard_led_blink();
    #endif

    multicore_launch_core1(periodic_wakeup);

    set_pins_as_output_for_dormant_wakeup();

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
    uart_lock = spin_lock_instance(UART_SPINLOCK_ID);

    if (watchdog_caused_reboot()){
        multicore_fifo_drain();
        sleep_ms(100);
        entry_point();
    }else{
        entry_point();
    }
}
