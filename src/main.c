#include "libopencm3/cm3/nvic.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/stm32/rcc.h"
#include "libopencm3/stm32/st_usbfs.h"
#include "libopencmsis/core_cm3.h"
#include "tusb.h"

#include "ramdisk.h"
#include "systick.h"
#include "target_flash.h"
#include "usb.h"

uint8_t app_valid = 0;

int main(void) {
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL,
                  GPIO0);
    gpio_set(GPIOC, GPIO0);

    if (gpio_get(GPIOC, GPIO0)) {
        volatile uint32_t const *app_vector =
            (volatile uint32_t const *)APPLICATION_ENTRY;

        rcc_periph_clock_disable(RCC_GPIOC);

        /* Jump to main */
        asm("MSR msp, %0" ::"r"(app_vector[0]));
        asm("BX %0" ::"r"(app_vector[1]));
    }

    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_set_usbpre(RCC_CFGR_USBPRE_PLL_CLK_DIV1_5);
    rcc_periph_clock_enable(RCC_USB);

    systick_init();
    ramdisk_init();

    tusb_init();

    while (1) {
        tud_task();

        if (app_valid) {
            scb_reset_system();
        }
    }
}
