/**
 * @file power_management.c
 * @brief Power-saving configuration and dormant mode handling for UART-based clients.
 *
 * This file contains functions for:
 * - Disabling unused clocks and peripherals to reduce power consumption
 * - Setting up GPIO pins for wake-up events from dormant mode
 * - Switching clock sources for low-power operation (ROSC, XOSC, LPOSC)
 * - Entering and exiting dormant mode on RP2040 or RP2350
 * - Restoring system state and UART after wake-up
 *
 * Supports both RP2040 and RP2350 platforms, with conditional configuration for timers,
 * power management units, and oscillator options.
 *
 * @note Used by UART clients to enter low-power state and wake up safely on RX activity.
 *
 * @see enter_dormant_mode()
 * @see wake_up()
 * @see sleep_run_from_dormant_source()
 * @see sleep_goto_dormant_until_pin()
 * @see power_saving_config()
 */

 #include "client.h"
#include "functions.h"

#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#include "hardware/regs/clocks.h"
#include "hardware/regs/rosc.h"
#include "hardware/structs/rosc.h"
#include "hardware/pll.h"
#include "hardware/xosc.h"
#include "pico/runtime_init.h"

#if !PICO_RP2040
#include "hardware/powman.h"
#endif

bool go_dormant_flag = false;
bool woke_up_from_dormant = false;

typedef enum {
    DORMANT_SOURCE_NONE,
    DORMANT_SOURCE_XOSC,
    DORMANT_SOURCE_ROSC,
    DORMANT_SOURCE_LPOSC, // rp2350 only
} dormant_source_t;

static dormant_source_t _dormant_source;

void client_turn_off_unused_power_consumers(void){
    clocks_hw->clk[clk_usb].ctrl &= ~CLOCKS_CLK_USB_CTRL_ENABLE_BITS;
    clocks_hw->clk[clk_adc].ctrl &= ~CLOCKS_CLK_ADC_CTRL_ENABLE_BITS;
    #if PICO_RP2040
        clocks_hw->clk[clk_rtc].ctrl &= ~CLOCKS_CLK_RTC_CTRL_ENABLE_BITS;
    #endif
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
        #if PICO_RP2040
            CLOCKS_SLEEP_EN0_CLK_SYS_VREG_AND_CHIP_RESET_BITS;
        #endif
        #if PICO_RP2350
            CLOCKS_SLEEP_EN0_CLK_SYS_GLITCH_DETECTOR_BITS;
        #endif

    #if PICO_RP2040
        clocks_hw->sleep_en1 = CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS;
    #endif 
    #if PICO_RP2350
        clocks_hw->sleep_en1 = CLOCKS_SLEEP_EN1_CLK_SYS_TIMER0_BITS | CLOCKS_SLEEP_EN1_CLK_SYS_TIMER1_BITS;
    #endif

    if (uart_get_index(active_uart_client_connection.uart_instance)){
        clocks_hw->sleep_en1 |=
        CLOCKS_SLEEP_EN1_CLK_SYS_UART1_BITS  |
        CLOCKS_SLEEP_EN1_CLK_PERI_UART1_BITS;
    }else{
        clocks_hw->sleep_en1 |=
        CLOCKS_SLEEP_EN1_CLK_SYS_UART0_BITS  |
        CLOCKS_SLEEP_EN1_CLK_PERI_UART0_BITS;
    }
}

static void set_pin_as_input_for_dormant_wakeup(void){
    uint8_t pin = active_uart_client_connection.pin_pair.tx;
    gpio_deinit(pin);
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, false, true);
}

void power_saving_config(void){
    client_turn_off_unused_power_consumers();

    uart_init_with_single_pin(active_uart_client_connection.uart_instance,
        active_uart_client_connection.pin_pair.rx,
        DEFAULT_BAUDRATE);

    set_pin_as_input_for_dormant_wakeup();
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

static void rosc_disable(void) {
    uint32_t tmp = rosc_hw->ctrl;
    tmp &= (~ROSC_CTRL_ENABLE_BITS);
    tmp |= (ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB);
    rosc_write(&rosc_hw->ctrl, tmp);
    // Wait for stable to go away
    while(rosc_hw->status & ROSC_STATUS_STABLE_BITS);
}

static void rosc_set_dormant(void) {
    // WARNING: This stops the rosc until woken up by an irq
    rosc_write(&rosc_hw->dormant, ROSC_DORMANT_VALUE_DORMANT);
    // Wait for it to become stable once woken up
    while(!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

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
static bool dormant_source_valid(dormant_source_t dormant_source)
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

static void _go_dormant(void) {
    assert(dormant_source_valid(_dormant_source));

    if (_dormant_source == DORMANT_SOURCE_XOSC) {
        xosc_dormant();
    } else {
        rosc_set_dormant();
    }
}

/**
 * @brief Prepares the system clocks for low-power dormant wake-up and reinitializes UART.
 *
 * Configures the clock sources and stops unused clocks (USB, ADC, etc.) to allow
 * the RP2040 to safely enter and resume from dormant mode. Selects a low-power 
 * clock source (XOSC or ROSC), disables the unused oscillator, and reinitializes
 * the default UART with the new clock configuration.
 *
 * @param dormant_source The clock source to be used during and after dormant mode.
 *                       Supported values:
 *                       - DORMANT_SOURCE_XOSC (external crystal oscillator)
 *                       - DORMANT_SOURCE_ROSC (ring oscillator)
 *                       - DORMANT_SOURCE_LPOSC (low-power oscillator, if available)
 *
 * @note This function updates global `_dormant_source`, disables PLLs, and stops
 *       clocks not needed in low-power state. It must be called **before** entering dormant mode.
 *
 * @warning The function assumes both XOSC and ROSC are active initially. It will
 *          disable the unused oscillator depending on the selected source.
 *
 * @see setup_default_uart()
 */
static void sleep_run_from_dormant_source(dormant_source_t dormant_source) {
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

/**
 * @brief Puts the system into dormant mode until a specified GPIO pin triggers a wake-up event.
 *
 * Configures a GPIO pin as the wake-up source using either edge or level detection,
 * then enters dormant mode. Execution resumes only when the pin event occurs.
 * After wake-up, the interrupt is acknowledged and the pin is deactivated.
 *
 * @param gpio_pin The GPIO number to monitor (must be < NUM_BANK0_GPIOS).
 * @param edge     If true, the wake-up will occur on edge detection (rising/falling).
 *                 If false, level detection (high/low) is used instead.
 * @param high     Determines the polarity:
 *                 - If true: rising edge or high level
 *                 - If false: falling edge or low level
 *
 * @note This function blocks until the wake-up event is triggered on the specified pin.
 *
 * @warning The pin must not be driven with unstable signals during dormant mode,
 *          or false wake-ups may occur.
 *
 * @see gpio_set_dormant_irq_enabled()
 * @see _go_dormant()
 */
static void sleep_goto_dormant_until_pin(uint gpio_pin, bool edge, bool high) {
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

void enter_dormant_mode(void){
    //reset_gpio_pins(active_uart_client_connection.pin_pair);
    sleep_run_from_dormant_source(DORMANT_SOURCE_ROSC);
    sleep_goto_dormant_until_pin(active_uart_client_connection.pin_pair.tx, false, true);
}

static void rosc_enable(void) {
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);
    while (!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));
}

/**
 * @brief Restores system clocks and peripherals after wake-up from dormant mode.
 *
 * Re-enables the ring oscillator (ROSC), resets the sleep enable registers,
 * reinitializes all clock domains, and restores the UART for standard output.
 * On RP2350, also reconfigures the power management timer to use XOSC as source.
 *
 * @note This function must be called after waking from dormant mode to restore
 *       full functionality of peripherals and system clocks.
 *
 * @warning Failure to call this function after wake-up may result in malfunctioning
 *          peripherals or missing UART output.
 *
 * @see clocks_init()
 * @see setup_default_uart()
 */
static void sleep_power_up(void)
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

void wake_up(void){
    sleep_power_up();
    client_turn_off_unused_power_consumers();

    uart_init_with_single_pin(active_uart_client_connection.uart_instance,
            active_uart_client_connection.pin_pair.rx,
            DEFAULT_BAUDRATE
    );

    set_pin_as_input_for_dormant_wakeup();
}



