#include "functions.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/**
 * @brief Initializes the onboard LED, depending on available hardware.
 *
 * - If using CYW43 Wi-Fi chip: initializes via `cyw43_arch_init()`.
 * - If using default GPIO LED: configures the pin as output.
 *
 * @return int Returns `PICO_OK` on success, or `-1` if no LED config is found.
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
 * @brief Reads from UART into a buffer with timeout and optional expected size.
 *
 * This function reads characters from UART until either:
 * - the timeout expires,
 * - the expected number of characters are received,
 * - or the buffer limit is hit.
 *
 * @param uart UART instance to read from.
 * @param buf Buffer where characters will be stored.
 * @param buffer_size Maximum size of the buffer.
 * @param expected_size Minimum number of characters to wait for.
 * @param timeout_ms Timeout in milliseconds.
 */
void get_uart_buffer(uart_inst_t* uart, char* buf, uint8_t buffer_size, uint8_t expected_size, uint32_t timeout_ms) {
    absolute_time_t start_time = get_absolute_time();
    uint8_t idx = 0;
    uint32_t timeout_us = timeout_ms * MS_TO_US_MULTIPLIER;

    while (absolute_time_diff_us(start_time, get_absolute_time()) < timeout_us) {
        if (uart_is_readable(uart)) {
            char c = uart_getc(uart);
            if (idx < buffer_size - 1) {
                buf[idx++] = c;
            } 
            else {
                break;
            }
            if (idx >= expected_size) break;
        }
    }
    printf("BUFFER: '%s'\n", buf);
    buf[idx] = '\0';  
}

/**
 * @brief Reads a response from the UART and checks for `CONNECTION_ACCEPTED_MESSAGE`.
 *
 * @param uart_instance The UART instance to read from.
 * @param timeout_ms Timeout for the response.
 * @return true if the expected message is received, false otherwise.
 */
bool uart_server_read_response(uart_inst_t* uart_instance, uint32_t timeout_ms){
    char buf[32] = {0};
    get_uart_buffer(uart_instance, buf, sizeof(buf), strlen(CONNECTION_ACCEPTED_MESSAGE), timeout_ms);

    printf("[DEBUG-SERVER] RECEIVED:\n'%s'\n", buf);

    if (strncmp(buf, CONNECTION_ACCEPTED_MESSAGE, strlen(CONNECTION_ACCEPTED_MESSAGE)) == 0){
        return true;
    }
    return false;
}


/**
 * @brief Server reads a connection request and sends an echo back to the client.
 *
 * - Matches the message against the expected request format.
 * - Echoes back the pair (e.g., "[4,5]").
 * - Waits for confirmation (CONNECTION_ACCEPTED_MESSAGE).
 *
 * @param uart_instance UART instance used.
 * @param expected Expected prefix message (e.g. "Requesting Connection").
 * @param timeout_ms Timeout in milliseconds.
 * @return true if handshake completed, false otherwise.
 */
bool uart_server_read(uart_inst_t* uart_instance, const char* expected, uint32_t timeout_ms){
    char buf[32] = {0};
    get_uart_buffer(uart_instance, buf, sizeof(buf), strlen(expected) + strlen("-[t,r]"), timeout_ms);

    if (strlen(buf) > 1){
        printf("[DEBUG-SERVER] RECEIVED:\n'%s'\n", buf);
        char received_tx_number = buf[strlen(CONNECTION_ACCEPTED_MESSAGE) + 4];
        char received_rx_number = buf[strlen(CONNECTION_ACCEPTED_MESSAGE) + 6];

        char received_pair[6];
        snprintf(received_pair, sizeof(received_pair), "[%c,%c]", received_tx_number, received_rx_number);
        uart_puts(uart_instance, received_pair);
        printf("[DEBUG-SERVER] Sending:\n'%s'\n", received_pair);
    }
    else{
        printf("[DEBUG-SERVER] BUFFER EMPTY\n");
    }

    if (uart_server_read_response(uart_instance, timeout_ms)){
        return true;
    }

    return false;
}


/**
 * @brief Client waits for echo from server and validates the pin pair.
 *
 * Compares the received echo pair with the one it originally sent. If they match,
 * sends back `CONNECTION_ACCEPTED_MESSAGE`.
 *
 * @param uart_instance UART peripheral in use.
 * @param pin_pair The TX/RX pair originally tested.
 * @param timeout_ms Timeout for the response.
 * @return true if the server echo is valid and ACK is sent, false otherwise.
 */
bool uart_client_read(uart_inst_t* uart_instance, uart_pin_pair_t pin_pair, uint32_t timeout_ms){
    char buf[6] = {0};
    get_uart_buffer(uart_instance, buf, sizeof(buf), strlen("[t,r]"), timeout_ms);

    printf("[DEBUG-CLIENT] RECEIVED:\n'%s'\n", buf);

    int expected_tx_number = buf[1] - '0';
    int expected_rx_number = buf[3] - '0';

    if (expected_tx_number == pin_pair.tx && expected_rx_number == pin_pair.rx){
        uart_puts(uart_instance, CONNECTION_ACCEPTED_MESSAGE);
        return true;
    }

    return false;
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