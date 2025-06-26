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
    int blink_iterations = 5;
    while (blink_iterations--){
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
    }
}

/**
 * @brief Reads incoming UART data into a buffer with timeout support.
 *
 * This function reads characters from the UART until:
 * - The buffer is full,
 * - The character ']' is received,
 * - Or the timeout expires.
 *
 * @param uart UART instance to read from.
 * @param buf Pointer to the buffer to store the received characters.
 * @param buffer_size Size of the buffer (must include space for null terminator).
 * @param timeout_ms Timeout duration in milliseconds.
 */
void get_uart_buffer(uart_inst_t* uart, char* buf, uint8_t buffer_size, uint32_t timeout_ms) {
    absolute_time_t start_time = get_absolute_time();
    uint8_t idx = 0;
    uint32_t timeout_us = timeout_ms * MS_TO_US_MULTIPLIER;

    if (uart_is_readable(uart)) {
        char c = uart_getc(uart);
    }

    while (absolute_time_diff_us(start_time, get_absolute_time()) < timeout_us) {
        if (uart_is_readable(uart)) {
            char c = uart_getc(uart);
            if (idx < buffer_size - 1) {
                buf[idx++] = c;
            } 
            else {
                break;
            }

            if (c == ']'){
                break;
            } 
        }
    }

    buf[idx] = '\0';  
}

/**
 * @brief Extracts two decimal numbers from a string of the form "[x,y]".
 *
 * This function scans the buffer and parses two integers separated by a comma.
 * The numbers are written into the provided array.
 *
 * @param received_number_pair Pointer to a 2-element array to hold the parsed numbers.
 * @param buf Input buffer containing the formatted string.
 */
void get_number_pair(uint8_t *received_number_pair, char *buf){
    char *p = buf;
    uint8_t number_pair_array_index = 0;

    while (*p) {
        if (*p - '0' >= 0 && *p - '0' <= 9) {
            received_number_pair[number_pair_array_index] = received_number_pair[number_pair_array_index] * 10 + (*p - '0');
        } else if (*p == ',') {
            number_pair_array_index++;
        }
        p++;
    }
}

/**
 * @brief Server-side handshake logic: responds to connection requests and validates client ACK.
 *
 * - Reads a request of the form "Requesting Connection-[tx,rx]".
 * - Sends back an echo of the pin pair in the format "[tx,rx]".
 * - Waits for and validates the client's ACK: "[Connection Accepted]".
 *
 * @param uart_instance UART interface used for communication.
 * @param timeout_ms Timeout in milliseconds for each stage.
 * @return true if a complete and valid handshake occurs, false otherwise.
 */
bool uart_server_read(uart_inst_t* uart_instance, uint32_t timeout_ms){
    char buf[32] = {0};
    uint8_t received_number_pair[2] = {0};

    sleep_ms(10);
    get_uart_buffer(uart_instance, buf, sizeof(buf), timeout_ms);
    
    if (strlen(buf) > 1){
        get_number_pair(received_number_pair, buf);
        uint8_t received_tx_number = received_number_pair[0];
        uint8_t received_rx_number = received_number_pair[1];

        char received_pair[8];
        snprintf(received_pair, sizeof(received_pair), "[%d,%d]", received_tx_number, received_rx_number);
        uart_puts(uart_instance, received_pair);
    }
    else{
        return false;
    }

    char ack_buf[32] = {0};
    get_uart_buffer(uart_instance, ack_buf, sizeof(ack_buf), timeout_ms);

    if (strcmp(ack_buf, "[" CONNECTION_ACCEPTED_MESSAGE "]") == 0){
        return true;
    }

    return false;
}



/**
 * @brief Client-side handshake logic: validates server echo and sends ACK.
 *
 * - Waits for server to echo the pin pair.
 * - Compares it to the pin pair originally sent.
 * - Sends "[Connection Accepted]" if the echo is valid.
 *
 * @param uart_instance UART interface used for communication.
 * @param pin_pair The TX/RX pair originally tested.
 * @param timeout_ms Timeout in milliseconds for the response.
 * @return true if the handshake is successful, false otherwise.
 */
bool uart_client_read(uart_inst_t* uart_instance, uart_pin_pair_t pin_pair, uint32_t timeout_ms){
    char buf[32] = {0};
    uint8_t received_number_pair[2] = {0};

    sleep_ms(10); 
    get_uart_buffer(uart_instance, buf, sizeof(buf), timeout_ms);
    get_number_pair(received_number_pair, buf);

    uint8_t expected_tx_number = received_number_pair[0];
    uint8_t expected_rx_number = received_number_pair[1];

    if (expected_tx_number == pin_pair.tx && expected_rx_number == pin_pair.rx){
        char accepted[strlen(CONNECTION_ACCEPTED_MESSAGE) + 3];
        snprintf(accepted, sizeof(accepted), "[%s]", CONNECTION_ACCEPTED_MESSAGE);
        sleep_ms(5);  
        
        uart_puts(uart_instance, accepted);
        return true;
    }

    return false;
}


/**
 * @brief Initializes UART and configures TX/RX GPIO pins.
 *
 * - Deinitializes the UART first to reset state.
 * - Assigns UART function to the specified TX and RX pins.
 * - Initializes the UART with the provided baud rate.
 *
 * @param uart UART instance (e.g., uart0 or uart1).
 * @param pin_pair The TX/RX pin pair to configure.
 * @param baudrate UART baud rate (e.g., 9600, 115200).
 */
void uart_init_with_pins(uart_inst_t* uart, uart_pin_pair_t pin_pair, uint32_t baudrate){
    uart_deinit(uart);
    gpio_set_function(pin_pair.tx, GPIO_FUNC_UART);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_UART);
    uart_init(uart, baudrate);
}
