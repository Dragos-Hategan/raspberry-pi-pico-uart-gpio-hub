/**
 * @file main.c
 * @brief UART client application for Raspberry Pi Pico.
 *
 * This application runs on a Raspberry Pi Pico acting as a UART client.
 * It scans all predefined TX/RX pin combinations for both UART0 and UART1
 * in order to identify a valid UART connection with a server.
 *
 * Once a successful handshake is established, the connection details are stored,
 * and the client begins listening for GPIO control commands from the server.
 */

#include <stdbool.h>

#include "client.h"
#include "functions.h"

#include <stdio.h>
#include "hardware/clocks.h"
#include "hardware/pll.h"

static void client_turn_off_unused_power_consumers(void){
    clocks_hw->clk[clk_usb].ctrl &= ~CLOCKS_CLK_USB_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_adc].ctrl &= ~CLOCKS_CLK_ADC_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_rtc].ctrl &= ~CLOCKS_CLK_RTC_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_gpout0].ctrl &= ~CLOCKS_CLK_GPOUT0_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_gpout1].ctrl &= ~CLOCKS_CLK_GPOUT1_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_gpout2].ctrl &= ~CLOCKS_CLK_GPOUT2_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_gpout3].ctrl &= ~CLOCKS_CLK_GPOUT3_CTRL_ENABLE_BITS;

    clock_configure(clk_ref,
        CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
        0, 12 * MHZ, 12 * MHZ);

    clock_configure(clk_sys,
        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
        0, 12 * MHZ, 12 * MHZ);

    clock_configure(clk_peri,
        0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
        12 * MHZ, 12 * MHZ); 

    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    clocks_hw->sleep_en0 =
        CLOCKS_SLEEP_EN0_CLK_SYS_SIO_BITS |
        CLOCKS_SLEEP_EN0_CLK_SYS_IO_BITS |
        CLOCKS_SLEEP_EN0_CLK_SYS_BUSFABRIC_BITS |
        CLOCKS_SLEEP_EN0_CLK_SYS_CLOCKS_BITS |
        CLOCKS_SLEEP_EN0_CLK_SYS_VREG_AND_CHIP_RESET_BITS;

    if (uart_get_index(active_uart_client_connection.uart_instance)){
        clocks_hw->sleep_en1 =
        CLOCKS_SLEEP_EN1_CLK_SYS_UART1_BITS  |
        CLOCKS_SLEEP_EN1_CLK_PERI_UART1_BITS |
        CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS;
    }else{
        clocks_hw->sleep_en1 =
        CLOCKS_SLEEP_EN1_CLK_SYS_UART0_BITS  |
        CLOCKS_SLEEP_EN1_CLK_PERI_UART0_BITS |
        CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS;
    }

    uart_init_with_pins(active_uart_client_connection.uart_instance,
            active_uart_client_connection.pin_pair,
            DEFAULT_BAUDRATE
    );
}

/**
 * @brief Entry point for the UART client application.
 *
 * Initializes the onboard LED and USB interface, then waits for a valid UART
 * connection with the server. Once connected, the client restores the last
 * known GPIO states and enters the main loop to listen for further commands.
 *
 * @return Unused. This function never returns.
 */
int main(void){
    init_onboard_led_and_usb();

    while(!client_detect_uart_connection()) tight_loop_contents();

    client_turn_off_unused_power_consumers();

    client_listen_for_commands();
}

