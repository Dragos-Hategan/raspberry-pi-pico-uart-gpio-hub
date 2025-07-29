/**
 * @file functions.c
 * @brief Common utility functions for UART communication and LED control.
 *
 * This file contains shared helper functions used across the project,
 * such as UART buffer parsing, LED signaling, and UART initialization.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdio_usb.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"
#include "hardware/regs/rosc.h"
#include "hardware/structs/rosc.h"
#include "pico/runtime_init.h"

#include "functions.h"
#include "config.h"

#if !PICO_RP2040
#include "hardware/powman.h"
#endif

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

void uart_init_with_pins(uart_inst_t* uart, uart_pin_pair_t pin_pair, uint32_t baudrate){
    uart_deinit(uart);
    gpio_set_function(pin_pair.tx, GPIO_FUNC_UART);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_UART);
    uart_init(uart, baudrate);
}

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

int pico_onboard_led_init(void) {
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

void pico_set_onboard_led(bool led_on) {
    #if defined(CYW43_WL_GPIO_LED_PIN)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
    #elif defined(PICO_DEFAULT_LED_PIN)
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);    
    #endif
}

void blink_onboard_led(void){
    int blink_iterations = 5;
    while (blink_iterations--){
        pico_set_onboard_led(false);
        sleep_ms(LED_DELAY_MS);
        pico_set_onboard_led(true);
        sleep_ms(LED_DELAY_MS);
    }
    pico_set_onboard_led(false);
}

void fast_blink_onboard_led(void){
    pico_set_onboard_led(true);
    sleep_ms(FAST_LED_DELAY_MS);
    pico_set_onboard_led(false);
}

void init_onboard_led_and_usb(void){
    pico_onboard_led_init();
    pico_set_onboard_led(true);    
    stdio_usb_init();
}

void sleep_run_from_rosc() {
    uint src_hz;
    src_hz = 6500000;

    clock_configure(clk_ref,
                    CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH,
                    0,
                    src_hz,
                    src_hz);

    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
                    0, 
                    src_hz,
                    src_hz);

    clock_stop(clk_adc);
    clock_stop(clk_usb);
    #if PICO_RP2350
        clock_stop(clk_hstx);
    #endif

    #if PICO_RP2040
        uint clk_rtc_src = CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;

        clock_configure(clk_rtc,
                        0,
                        clk_rtc_src,
                        src_hz,
                        46875);
    #endif

    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    src_hz,
                    src_hz);

    pll_deinit(pll_sys);
    pll_deinit(pll_usb);
    
    xosc_disable();

    setup_default_uart();
}

inline static void rosc_clear_bad_write(void) {
    hw_clear_bits(&rosc_hw->status, ROSC_STATUS_BADWRITE_BITS);
}

inline static bool rosc_write_okay(void) {
    return !(rosc_hw->status & ROSC_STATUS_BADWRITE_BITS);
}

inline static void rosc_write(io_rw_32 *addr, uint32_t value) {
    rosc_clear_bad_write();
    assert(rosc_write_okay());
    *addr = value;
    assert(rosc_write_okay());
};

static void go_dormant(void) {
    rosc_write(&rosc_hw->dormant, ROSC_DORMANT_VALUE_DORMANT);
    while(!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

void sleep_goto_dormant_until_pin(uint gpio_pin) {
    assert(gpio_pin < NUM_BANK0_GPIOS);

    gpio_init(gpio_pin);
    gpio_set_input_enabled(gpio_pin, true);
    gpio_set_dormant_irq_enabled(gpio_pin, IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_HIGH_BITS, true);

    go_dormant();

    gpio_acknowledge_irq(gpio_pin, IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_HIGH_BITS);
    gpio_set_input_enabled(gpio_pin, false);
}

void rosc_enable(void) {
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
    while (!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

void sleep_power_up(void)
{
    rosc_enable();

    clocks_hw->sleep_en0 |= ~(0u);
    clocks_hw->sleep_en1 |= ~(0u);

    clocks_init();

    #if PICO_RP2350
        uint64_t restore_ms = powman_timer_get_ms();
        powman_timer_set_1khz_tick_source_xosc();
        powman_timer_set_ms(restore_ms);
    #endif

    setup_default_uart();
}