#include "functions.h"
#include "config.h"
#include <string.h>

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/**
 * @brief Initializes the onboard LED for either CYW43 (Wi-Fi chip) or the default GPIO LED.
 * 
 * @return int Returns PICO_OK (0) if initialization was successful, -1 otherwise.
 */
int pico_led_init(void) {
#if defined(CYW43_WL_GPIO_LED_PIN)
    return cyw43_arch_init();
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#else
    return -1;
#endif
}

/**
 * @brief Turns the onboard LED on or off.
 * 
 * @param led_on true to turn on the LED, false to turn it off
 */
void pico_set_led(bool led_on) {
#if defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);    
#endif
}

/**
 * @brief Blinks the onboard LED 5 times with a defined delay.
 */
void blink_onboard_led(void){
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    int blink_iterations = 5;
    while (blink_iterations--){
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}

/**
 * @brief Reads from UART until a specific expected string is matched or timeout is reached.
 * 
 * @param uart_instance The UART instance (uart0 or uart1).
 * @param expected The expected string to match.
 * @param timeout_ms Timeout in milliseconds.
 * @return true if the expected string was matched, false otherwise.
 */
bool uart_read_until_match(uart_inst_t* uart_instance, const char* expected, uint32_t timeout_ms){
    absolute_time_t start_time = get_absolute_time();
    char buf[32] = {0};
    size_t idx = 0;

    uint32_t timeout_us = timeout_ms * MS_TO_US_MULTIPLIER;
    uint32_t passed_time = absolute_time_diff_us(start_time, get_absolute_time());

    while (passed_time < timeout_us) {
        if (uart_is_readable(uart_instance)) {
            char c = uart_getc(uart_instance);

            if (idx < sizeof(buf) - 1) {
                buf[idx++] = c;
                buf[idx] = '\0';
            } else {
                break; // overflow safety
            }
            if (idx >= strlen(expected)) break;

        }
        passed_time = absolute_time_diff_us(start_time, get_absolute_time());
    }

    return (strncmp(buf, expected, strlen(expected)) == 0);
}

/**
 * @brief Initializes UART with given TX/RX pins and baudrate.
 * 
 * @param uart The UART instance (e.g., uart0, uart1).
 * @param pin_pair The TX and RX pin numbers.
 * @param baudrate The baud rate for UART communication.
 */
void uart_init_with_pins(uart_inst_t* uart, uart_pin_pair_t pin_pair, uint32_t baudrate){
    uart_deinit(uart);
    gpio_set_function(pin_pair.tx, GPIO_FUNC_UART);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_UART);
    uart_init(uart, baudrate);
}