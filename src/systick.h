#if !defined(__SYSTICK_H)
#define __SYSTICK_H
#include <stdint.h>

void systick_init(void);
void delay(uint32_t ticks);

#endif // __SYSTICK_H
