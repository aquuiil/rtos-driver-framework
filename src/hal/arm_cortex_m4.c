#include "hal/arm_cortex_m4.h"
#include "kernel/scheduler.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
static HANDLE systick_thread = NULL;
static volatile bool systick_running = false;
#endif

void hal_init(void) {
    printf("[HAL] ARM Cortex-M4 HAL initialized (simulated)\n");
    /* Real HW: Configure system clock, enable FPU, etc. */
}

void hal_enable_interrupts(void) {
    /* Real HW: __enable_irq(); or CPSIE I */
}

void hal_disable_interrupts(void) {
    /* Real HW: __disable_irq(); or CPSID I */
}

#ifdef _WIN32
DWORD WINAPI systick_thread_func(LPVOID param) {
    (void)param;
    while (systick_running) {
        Sleep(1);  // 1ms tick
        scheduler_tick();
    }
    return 0;
}
#endif

void hal_systick_init(uint32_t ticks) {
    printf("[HAL] SysTick initialized: %u ticks\n", ticks);
    
    #ifdef _WIN32
    systick_running = true;
    systick_thread = CreateThread(NULL, 0, systick_thread_func, NULL, 0, NULL);
    #endif
    
    /* Real HW:
     * SysTick->LOAD = ticks - 1;
     * SysTick->VAL = 0;
     * SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | 
     *                 SysTick_CTRL_TICKINT_Msk | 
     *                 SysTick_CTRL_ENABLE_Msk;
     */
}

uint32_t hal_get_systick(void) {
    return scheduler_get_tick_count();
}