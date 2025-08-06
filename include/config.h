/**
 * @file config.h
 * @brief Project-wide configuration constants for UART GPIO server.
 *
 * Contains default settings like baud rate, UART pin limits, timeout values,
 * and storage constants used across server and client logic.
 */

#ifndef CONFIG_H
#define CONFIG_H


// === Enable Periodic Blink ===
#ifndef PERIODIC_ONBOARD_LED_BLINK_SERVER
#define PERIODIC_ONBOARD_LED_BLINK_SERVER 1
#endif

#ifndef PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS
#define PERIODIC_ONBOARD_LED_BLINK_ALL_CLIENTS 1
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
/// Minimum effective value is ~350ms. 500ms provides more robustness.
#ifndef SERVER_TIMEOUT_MS
#define SERVER_TIMEOUT_MS 500
#endif

/// Timeout in milliseconds for sending a UART connection request.
#ifndef CLIENT_TIMEOUT_MS
#define CLIENT_TIMEOUT_MS 50
#endif

#ifndef PERIODIC_ONBOARD_LED_BLINK_TIME_MS
#define PERIODIC_ONBOARD_LED_BLINK_TIME_MS 2500
#endif

#ifndef PERIODIC_CONSOLE_CHECK_TIME_MS
#define PERIODIC_CONSOLE_CHECK_TIME_MS 1500
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
#ifndef TRIGGER_RESET_FLAG_NUMBER
#define TRIGGER_RESET_FLAG_NUMBER 77
#endif

#ifndef BLINK_ONBOARD_LED_FLAG_NUMBER
#define BLINK_ONBOARD_LED_FLAG_NUMBER 66
#endif

#ifndef WAKE_UP_FLAG_NUMBER
#define WAKE_UP_FLAG_NUMBER 55
#endif

#ifndef DORMANT_FLAG_NUMBER
#define DORMANT_FLAG_NUMBER 44
#endif

#endif