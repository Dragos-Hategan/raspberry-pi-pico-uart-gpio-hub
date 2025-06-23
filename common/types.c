#include "types.h"
#include "config.h"

// make a .json/.ini/CSV file instead of hardcoding these variables
const uart_pin_pair_t pin_pairs_uart0[PIN_PAIRS_UART0_LEN] = {
    {0, 1},
    {12, 13},
    {16, 17}
};

const uart_pin_pair_t pin_pairs_uart1[PIN_PAIRS_UART1_LEN] = {
    {4, 5},
    {8, 9}
};
