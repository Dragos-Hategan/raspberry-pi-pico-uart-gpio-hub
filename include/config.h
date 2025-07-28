/**
 * @file config.h
 * @brief Project-wide configuration constants for UART GPIO server.
 *
 * Contains default settings like baud rate, UART pin limits, timeout values,
 * and storage constants used across server and client logic.
 */

#ifndef CONFIG_H
#define CONFIG_H


#ifndef RESTART_SYSTEM_AT_USB_RECONNECTION
#define RESTART_SYSTEM_AT_USB_RECONNECTION 0
#endif

// === UART Settings ===
#ifndef DEFAULT_BAUDRATE
#define DEFAULT_BAUDRATE 115200u
#endif

#ifndef CONNECTION_REQUEST_MESSAGE
#define CONNECTION_REQUEST_MESSAGE "Requesting Connection"
#endif

#ifndef CONNECTION_ACCEPTED_MESSAGE
#define CONNECTION_ACCEPTED_MESSAGE "Connection Accepted"
#endif


// === UART Connection Scan ===
#ifndef PIN_PAIRS_UART0_LEN
#define PIN_PAIRS_UART0_LEN 3
#endif

#ifndef PIN_PAIRS_UART1_LEN
#define PIN_PAIRS_UART1_LEN 2
#endif

#ifndef MAX_SERVER_CONNECTIONS
#define MAX_SERVER_CONNECTIONS (PIN_PAIRS_UART0_LEN + PIN_PAIRS_UART1_LEN)
#endif


// === Timings (ms) ===
#ifndef LED_DELAY_MS
#define LED_DELAY_MS 125
#endif

#ifndef FAST_LED_DELAY_MS
#define FAST_LED_DELAY_MS 25
#endif

/// Timeout in milliseconds for receiving a UART connection request.
/// Minimum effective value is ~300ms. 500ms provides more robustness.
#ifndef SERVER_TIMEOUT_MS
#define SERVER_TIMEOUT_MS 500
#endif

/// Timeout in milliseconds for sending a UART connection request.
#ifndef CLIENT_TIMEOUT_MS
#define CLIENT_TIMEOUT_MS 50
#endif

#ifndef MS_TO_US_MULTIPLIER
#define MS_TO_US_MULTIPLIER 1000
#endif


// === GPIO State ===
#ifndef MAX_NUMBER_OF_GPIOS
#define MAX_NUMBER_OF_GPIOS 26
#endif

#ifndef UART_CONNECTION_FLAG_NUMBER
#define UART_CONNECTION_FLAG_NUMBER 99
#endif

#define NUMBER_OF_POSSIBLE_PRESETS 5


// === Messages ===
#ifndef EMPTY_MEMORY_MESSAGE
#define EMPTY_MEMORY_MESSAGE "Empty"
#endif

#ifndef TRIGGER_RESET_FLAG_NUMBER
#define TRIGGER_RESET_FLAG_NUMBER 77
#endif

#ifndef BLINK_ONBOARD_LED_FLAG_NUMBER
#define BLINK_ONBOARD_LED_FLAG_NUMBER 66
#endif

#endif