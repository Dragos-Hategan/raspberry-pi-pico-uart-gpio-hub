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

#include "client.h"
#include "functions.h"
#include "config.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#if !PICO_RP2040
#include "hardware/powman.h"
#endif

void uart_init_with_pins(uart_inst_t* uart, uart_pin_pair_t pin_pair, uint32_t baudrate){
    uart_deinit(uart);
    gpio_set_function(pin_pair.tx, GPIO_FUNC_UART);
    gpio_set_function(pin_pair.rx, GPIO_FUNC_UART);
    uart_init(uart, baudrate);
    sleep_ms(1);
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
    printf("%s\n", buf);
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

void blink_onboard_led_blocking(void){
    int blink_iterations = 5;
    while (blink_iterations--){
        pico_set_onboard_led(false);
        sleep_ms(LED_DELAY_MS);
        pico_set_onboard_led(true);
        sleep_ms(LED_DELAY_MS);
    }
    pico_set_onboard_led(false);
}

int64_t my_alarm_cb(alarm_id_t id, void *user_data) {
    pico_set_onboard_led(false);
    return 0;
}

void fast_blink_onboard_led(void){
    pico_set_onboard_led(true);
    add_alarm_in_us(FAST_LED_DELAY_MS * 1000, my_alarm_cb, NULL, false);
}

void fast_blink_onboard_led_blocking(void){
    pico_set_onboard_led(true);
    sleep_ms(FAST_LED_DELAY_MS);
    pico_set_onboard_led(false);
}

void init_onboard_led_and_usb(void){
    pico_onboard_led_init();
    pico_set_onboard_led(true);    
    stdio_usb_init();
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
}

static void go_dormant(void) {
    rosc_write(&rosc_hw->dormant, ROSC_DORMANT_VALUE_DORMANT);
    while(!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

absolute_time_t t_start;
absolute_time_t t_stop;
absolute_time_t t_start2;
absolute_time_t t_stop2;
static dormant_source_t _dormant_source;

/**
 * @brief Checks if the specified dormant source is supported on the current platform.
 *
 * Validates whether the given `dormant_source_t` enum value represents a supported
 * oscillator source for entering dormant mode (e.g., XOSC, ROSC, or LPOSC on non-RP2040 platforms).
 *
 * @param dormant_source The dormant source to validate.
 * @return `true` if the source is supported on the current platform, `false` otherwise.
 *
 * @note LPOSC is only available on platforms other than RP2040.
 */
bool dormant_source_valid(dormant_source_t dormant_source)
{
    switch (dormant_source) {
        case DORMANT_SOURCE_XOSC:
            return true;
        case DORMANT_SOURCE_ROSC:
            return true;
#if !PICO_RP2040
        case DORMANT_SOURCE_LPOSC:
            return true;
#endif
        default:
            return false;
    }
}

void rosc_disable(void) {
    uint32_t tmp = rosc_hw->ctrl;
    tmp &= (~ROSC_CTRL_ENABLE_BITS);
    tmp |= (ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB);
    rosc_write(&rosc_hw->ctrl, tmp);
    // Wait for stable to go away
    while(rosc_hw->status & ROSC_STATUS_STABLE_BITS);
}

void rosc_set_dormant(void) {
    // WARNING: This stops the rosc until woken up by an irq
    rosc_write(&rosc_hw->dormant, ROSC_DORMANT_VALUE_DORMANT);
    // Wait for it to become stable once woken up
    while(!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

static void _go_dormant(void) {
    assert(dormant_source_valid(_dormant_source));

    if (_dormant_source == DORMANT_SOURCE_XOSC) {
        xosc_dormant();
    } else {
        rosc_set_dormant();
    }
}

void sleep_run_from_dormant_source(dormant_source_t dormant_source) {
    assert(dormant_source_valid(dormant_source));
    _dormant_source = dormant_source;

    uint src_hz;
    uint clk_ref_src;
    switch (dormant_source) {
        case DORMANT_SOURCE_XOSC:
            src_hz = XOSC_HZ;
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC;
            break;
        case DORMANT_SOURCE_ROSC:
            src_hz = 6500 * KHZ; // todo
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH;
            break;
#if !PICO_RP2040
        case DORMANT_SOURCE_LPOSC:
            src_hz = 32 * KHZ;
            clk_ref_src = CLOCKS_CLK_REF_CTRL_SRC_VALUE_LPOSC_CLKSRC;
            break;
#endif
        default:
            hard_assert(false);
    }

    // CLK_REF = XOSC or ROSC
    clock_configure(clk_ref,
                    clk_ref_src,
                    0, // No aux mux
                    src_hz,
                    src_hz);

    // CLK SYS = CLK_REF
    clock_configure(clk_sys,
                    CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF,
                    0, // Using glitchless mux
                    src_hz,
                    src_hz);

    // CLK ADC = 0MHz
    clock_stop(clk_adc);
    clock_stop(clk_usb);
#if PICO_RP2350
    clock_stop(clk_hstx);
#endif

#if PICO_RP2040
    // CLK RTC = ideally XOSC (12MHz) / 256 = 46875Hz but could be rosc
    uint clk_rtc_src = (dormant_source == DORMANT_SOURCE_XOSC) ?
                       CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC :
                       CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH;

    clock_configure(clk_rtc,
                    0, // No GLMUX
                    clk_rtc_src,
                    src_hz,
                    46875);
#endif

    // CLK PERI = clk_sys. Used as reference clock for Peripherals. No dividers so just select and enable
    clock_configure(clk_peri,
                    0,
                    CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                    src_hz,
                    src_hz);

    pll_deinit(pll_sys);
    pll_deinit(pll_usb);

    // Assuming both xosc and rosc are running at the moment
    if (dormant_source == DORMANT_SOURCE_XOSC) {
        // Can disable rosc
        rosc_disable();
    } else {
        // Can disable xosc
        xosc_disable();
    }

    // Reconfigure uart with new clocks
    setup_default_uart();
}

void sleep_goto_dormant_until_pin(uint gpio_pin, bool edge, bool high) {
    bool low = !high;
    bool level = !edge;

    // Configure the appropriate IRQ at IO bank 0
    assert(gpio_pin < NUM_BANK0_GPIOS);

    uint32_t event = 0;

    if (level && low) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_LOW_BITS;
    if (level && high) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_HIGH_BITS;
    if (edge && high) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_HIGH_BITS;
    if (edge && low) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_LOW_BITS;

    gpio_init(gpio_pin);
    gpio_set_input_enabled(gpio_pin, true);
    gpio_set_dormant_irq_enabled(gpio_pin, event, true);

    _go_dormant();
    // Execution stops here until woken up

    // Clear the irq so we can go back to dormant mode again if we want
    gpio_acknowledge_irq(gpio_pin, event);
    gpio_set_input_enabled(gpio_pin, false);
}

void rosc_enable(void) {
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
    while (!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

// To be called after waking up from sleep/dormant mode to restore system clocks properly
void sleep_power_up(void)
{
    // Re-enable the ring oscillator, which will essentially kickstart the proc
    rosc_enable();

    // Reset the sleep enable register so peripherals and other hardware can be used
    clocks_hw->sleep_en0 |= ~(0u);
    clocks_hw->sleep_en1 |= ~(0u);

    // Restore all clocks
    clocks_init();

#if PICO_RP2350
    // make powerman use xosc again
    uint64_t restore_ms = powman_timer_get_ms();
    powman_timer_set_1khz_tick_source_xosc();
    powman_timer_set_ms(restore_ms);
#endif

    // UART needs to be reinitialised with the new clock frequencies for stable output
    setup_default_uart();
}
