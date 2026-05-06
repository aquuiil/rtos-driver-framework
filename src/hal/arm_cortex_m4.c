/**
 * @file arm_cortex_m4.c
 * @brief ARM Cortex-M4 Hardware Abstraction Layer
 * 
 * Provides HAL abstraction for both:
 * 1. Windows simulation (thread-based SysTick)
 * 2. QEMU/ARM target (real SysTick register setup)
 */

#include "hal/arm_cortex_m4.h"
#include "kernel/scheduler.h"
#include <stdio.h>
#include <stdbool.h>

/* ==================== TARGET-SPECIFIC INCLUDES ==================== */

#ifdef TARGET_WINDOWS
#include <windows.h>
static HANDLE systick_thread = NULL;
static volatile bool systick_running = false;
#endif

#ifdef TARGET_QEMU
/* STM32F429 memory-mapped register bases */
#define SYSTICK_BASE        0xE000E010
#define SYSTICK_CTRL        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))
#define SYSTICK_CALIB       (*(volatile uint32_t *)(SYSTICK_BASE + 0x0C))

/* SysTick control register bits */
#define SYSTICK_CTRL_ENABLE     0x00000001
#define SYSTICK_CTRL_TICKINT    0x00000002
#define SYSTICK_CTRL_CLKSOURCE  0x00000004
#define SYSTICK_CTRL_COUNTFLAG  0x00010000

/* SCB base for priority registers */
#define SCB_BASE            0xE000ED00
#define SCB_ICSR            (*(volatile uint32_t *)(SCB_BASE + 0x04))
#define SCB_VTOR            (*(volatile uint32_t *)(SCB_BASE + 0x08))

/* NVIC for interrupt enable/disable */
#define NVIC_BASE           0xE000E100
#define NVIC_ISER(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x00 + (n)*4))
#define NVIC_ICER(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x80 + (n)*4))
#define NVIC_ISPR(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x100 + (n)*4))
#define NVIC_ICPR(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x180 + (n)*4))
#define NVIC_IABR(n)        (*(volatile uint32_t *)(NVIC_BASE + 0x200 + (n)*4))
#define NVIC_IPR(n)         (*(volatile uint32_t *)(NVIC_BASE + 0x300 + (n)*4))
#endif

/* ==================== GLOBAL STATE ==================== */

static uint32_t hal_tick_count = 0;

/* ==================== HAL INITIALIZATION ==================== */

void hal_init(void) {
#ifdef TARGET_WINDOWS
    printf("[HAL] Windows simulation mode - ARM Cortex-M4 HAL initialized\n");
#endif

#ifdef TARGET_QEMU
    printf("[HAL] QEMU/ARM target - Cortex-M4 HAL initialized\n");
    printf("[HAL] Configuring system clock and FPU...\n");
    
    /* Enable FPU (Cortex-M4 has hardware FPU) */
    uint32_t *cpacr = (uint32_t *)0xE000ED88;  /* CPACR register */
    *cpacr |= (0xF << 20);  /* Enable CP10 and CP11 for full FPU access */
    __asm volatile("dsb");
    __asm volatile("isb");
#endif
}

/* ==================== INTERRUPT CONTROL ==================== */

void hal_enable_interrupts(void) {
#ifdef TARGET_WINDOWS
    /* Windows simulation - nothing to do */
#endif

#ifdef TARGET_QEMU
    __asm volatile("cpsie i");  /* Set PRIMASK to allow interrupts */
#endif
}

void hal_disable_interrupts(void) {
#ifdef TARGET_WINDOWS
    /* Windows simulation - nothing to do */
#endif

#ifdef TARGET_QEMU
    __asm volatile("cpsid i");  /* Clear PRIMASK to disable interrupts */
#endif
}

/* ==================== SYSTICK INITIALIZATION ==================== */

#ifdef TARGET_WINDOWS
DWORD WINAPI systick_thread_func(LPVOID param) {
    (void)param;
    while (systick_running) {
        Sleep(1);  /* 1ms tick */
        hal_tick_count++;
        scheduler_tick();
    }
    return 0;
}
#endif

void hal_systick_init(uint32_t ticks) {
    printf("[HAL] Initializing SysTick: %u ticks per second\n", ticks);
    
#ifdef TARGET_WINDOWS
    /* Windows: Use thread-based timer (approximate) */
    systick_running = true;
    systick_thread = CreateThread(NULL, 0, systick_thread_func, NULL, 0, NULL);
    if (systick_thread == NULL) {
        printf("[HAL] ERROR: Failed to create SysTick thread\n");
    }
#endif

#ifdef TARGET_QEMU
    /**
     * STM32F429 system clock: 168 MHz
     * SysTick timer: 24-bit counter, clocked by AHB/8 = 21 MHz
     * For 1000 Hz (1ms tick): LOAD = (21MHz / 1000Hz) - 1 = 20999
     */
    
    /* Calculate reload value for desired tick rate */
    /* Assuming 168 MHz system clock, AHB/8 = 21 MHz */
    uint32_t reload_value = (21000000 / ticks) - 1;
    
    if (reload_value > 0xFFFFFF) {
        printf("[HAL] ERROR: SysTick reload value too large\n");
        reload_value = 0xFFFFFF;
    }
    
    /* Configure SysTick */
    SYSTICK_VAL = 0;  /* Clear current value */
    SYSTICK_LOAD = reload_value;  /* Set reload value */
    
    /* Enable SysTick with interrupt, use AHB/8 as clock source */
    SYSTICK_CTRL = SYSTICK_CTRL_ENABLE | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_CLKSOURCE;
    
    printf("[HAL] SysTick configured: reload=%d (%.2f ms per tick)\n",
           reload_value, (float)(reload_value + 1) / 21000000.0 * 1000.0);
#endif
}

/* ==================== TICK COUNTER ==================== */

uint32_t hal_get_systick(void) {
    return hal_tick_count;
}

/* ==================== SYSTICK INTERRUPT HANDLER ==================== */

/**
 * SysTick exception handler (Exception 15)
 * Called every SysTick period (1ms for 1000 Hz)
 * 
 * This handler increments the tick counter and calls the scheduler tick
 * to check for task wake-ups and preemption.
 */
void sys_tick_handler(void) {
    hal_tick_count++;
    scheduler_tick();
}