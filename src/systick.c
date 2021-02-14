#include "systick.h"

#include "libopencm3/cm3/systick.h"
#include "libopencm3/cm3/nvic.h"
#include "libopencm3/stm32/rcc.h"

static volatile uint32_t _system_tick = 0;

void systick_init(void) {
    systick_set_frequency(1000, rcc_ahb_frequency);
    nvic_enable_irq(NVIC_SYSTICK_IRQ);
    nvic_set_priority(NVIC_SYSTICK_IRQ, 0);
    systick_interrupt_enable();
    systick_counter_enable();
}

void sys_tick_handler(void) {
	_system_tick++;
}

void delay(uint32_t ticks) {
	uint32_t begin_tick = _system_tick;
	while (_system_tick <= begin_tick + ticks);

	return;
}
