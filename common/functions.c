#include "functions.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

// Perform initialisation
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

// Turn the led on or off
void pico_set_led(bool led_on) {
#if defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#elif defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);    
#endif
}

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

bool uart_read_until_match(uart_inst_t* uart_instance, const char* expected, uint timeout_ms){
    absolute_time_t start_time = get_absolute_time();
    absolute_time_t last_rx_time = start_time;
    bool started_receiving = false;
    char buf[32] = {0};
    size_t idx = 0;

    while (absolute_time_diff_us(get_absolute_time(), start_time) < timeout_ms * 1000) {
        if (uart_is_readable(uart_instance)) {
            char c = uart_getc(uart_instance);

            if (idx < sizeof(buf) - 1) {
                buf[idx++] = c;
                buf[idx] = '\0';
                last_rx_time = get_absolute_time();
                started_receiving = true;
            } else {
                break; // overflow safety
            }
            if (idx >= strlen(expected)) break;
        }
        else{
            if (started_receiving && absolute_time_diff_us(get_absolute_time(), last_rx_time) > timeout_ms * 1000){
                break;  // timeout between characters
            }
        }
    }

    return (strncmp(buf, expected, strlen(expected)) == 0);
}

void uart_init_with_pins(uart_inst_t* uart, uart_pin_pair_t pin_pair, uint baudrate){
    uart_deinit(uart);
    gpio_set_function(pin_pair.tx, GPIO_FUNC_UART);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_UART);
    uart_init(uart, baudrate);
}