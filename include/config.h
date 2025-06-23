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

#ifndef MAX_SERVER_CONNECTIONS
#define MAX_SERVER_CONNECTIONS 5
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

#ifndef MS_TO_US_MULTIPLIER
#define MS_TO_US_MULTIPLIER 1000
#endif


#endif