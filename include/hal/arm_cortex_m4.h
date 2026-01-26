#ifndef ARM_CORTEX_M4_H
#define ARM_CORTEX_M4_H

#include "rtos_types.h"

void hal_init(void);
void hal_enable_interrupts(void);
void hal_disable_interrupts(void);
void hal_systick_init(uint32_t ticks);
uint32_t hal_get_systick(void);

#endif /* ARM_CORTEX_M4_H */