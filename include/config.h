#ifndef CONFIG_H
#define CONFIG_H

#ifndef DEFAULT_BAUDRATE
#define DEFAULT_BAUDRATE 115200u
#endif

#ifndef CONNECTION_REQUEST_MESSAGE
#define CONNECTION_REQUEST_MESSAGE "Requesting Connection"
#endif

#ifndef CONNECTION_ACCEPTED_MESSAGE
#define CONNECTION_ACCEPTED_MESSAGE "Connection Accepted"
#endif

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 5
#endif

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 125
#endif

#ifndef PIN_PAIRS_UART0_LEN
#define PIN_PAIRS_UART0_LEN 3
#endif

#ifndef PIN_PAIRS_UART1_LEN
#define PIN_PAIRS_UART1_LEN 2
#endif

typedef struct{
    int tx;
    int rx;
}uart_pin_pair_t;

extern const uart_pin_pair_t pin_pairs_uart0[PIN_PAIRS_UART0_LEN];
extern const uart_pin_pair_t pin_pairs_uart1[PIN_PAIRS_UART1_LEN];

#endif