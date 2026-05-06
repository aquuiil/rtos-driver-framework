/**
 * @file stm32f429_startup.c
 * @brief STM32F429 startup code and interrupt vectors
 * 
 * Provides reset handler, interrupt vector table, and hardware initialization
 * for ARM Cortex-M4 target. Used when building with QEMU target.
 */

#include <stdint.h>

/* ==================== EXTERNAL SYMBOLS ==================== */

extern void hal_init(void);
extern int main(void);
extern void scheduler_init(void);

/* Forward declare stack pointer (from linker script) */
extern uint32_t _estack;

/* ==================== INTERRUPT HANDLERS ==================== */

/**
 * Default fault handler - catches unhandled exceptions
 */
void default_fault_handler(void) {
    while (1) {
        /* Hang - CPU will be stuck here if an unhandled exception occurs */
    }
}

/* System handlers (Exception 0-15) */
void reset_handler(void);           /* Exception 1  - Reset */
void nmi_handler(void) __attribute__((weak, alias("default_fault_handler")));
void hard_fault_handler(void) __attribute__((weak, alias("default_fault_handler")));
void mem_manage_fault_handler(void) __attribute__((weak, alias("default_fault_handler")));
void bus_fault_handler(void) __attribute__((weak, alias("default_fault_handler")));
void usage_fault_handler(void) __attribute__((weak, alias("default_fault_handler")));
void sv_call_handler(void) __attribute__((weak, alias("default_fault_handler")));
void debug_mon_handler(void) __attribute__((weak, alias("default_fault_handler")));
void pend_sv_handler(void) __attribute__((weak, alias("default_fault_handler")));
void sys_tick_handler(void) __attribute__((weak, alias("default_fault_handler")));

/* Peripheral handlers (Exception 16-) */
void wwdg_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void pvd_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tamp_stamp_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void rtc_wkup_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void flash_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void rcc_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void exti0_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void exti1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void exti2_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void exti3_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void exti4_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream0_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream2_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream3_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream4_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream5_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream6_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void adc_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can1_tx_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can1_rx0_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can1_rx1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can1_sce_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void exti9_5_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim1_brk_tim9_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim1_up_tim10_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim1_trg_com_tim11_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim1_cc_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim2_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim3_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim4_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void i2c1_ev_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void i2c1_er_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void i2c2_ev_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void i2c2_er_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void spi1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void spi2_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void usart1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void usart2_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void usart3_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void exti15_10_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void rtc_alarm_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void otg_fs_wkup_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim8_brk_tim12_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim8_up_tim13_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim8_trg_com_tim14_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim8_cc_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma1_stream7_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void fsmc_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void sdio_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim5_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void spi3_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void uart4_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void uart5_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim6_dac_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void tim7_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream0_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream2_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream3_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream4_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void eth_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void eth_wkup_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can2_tx_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can2_rx0_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can2_rx1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void can2_sce_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void otg_fs_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream5_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream6_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2_stream7_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void usart6_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void i2c3_ev_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void i2c3_er_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void otg_hs_ep1_out_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void otg_hs_ep1_in_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void otg_hs_wkup_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void otg_hs_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dcmi_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void cryp_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void hash_rng_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void fpu_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void uart7_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void uart8_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void spi4_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void spi5_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void spi6_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void sai1_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void lcd_tft_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void lcd_tft_err_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));
void dma2d_irq_handler(void) __attribute__((weak, alias("default_fault_handler")));

/* ==================== INTERRUPT VECTOR TABLE ==================== */

/**
 * Cortex-M4 interrupt vector table
 * Positioned at start of flash (0x08000000) and copied to RAM if needed
 */
__attribute__((section(".isr_vector"), used))
const uint32_t isr_vector[173] = {
    /* System exceptions */
    (uint32_t)&_estack,                         /* 0: Stack pointer */
    (uint32_t)reset_handler,                    /* 1: Reset handler */
    (uint32_t)nmi_handler,                      /* 2: NMI */
    (uint32_t)hard_fault_handler,               /* 3: Hard Fault */
    (uint32_t)mem_manage_fault_handler,         /* 4: Mem Manage */
    (uint32_t)bus_fault_handler,                /* 5: Bus Fault */
    (uint32_t)usage_fault_handler,              /* 6: Usage Fault */
    0,                                          /* 7: Reserved */
    0,                                          /* 8: Reserved */
    0,                                          /* 9: Reserved */
    0,                                          /* 10: Reserved */
    (uint32_t)sv_call_handler,                  /* 11: SVCall */
    (uint32_t)debug_mon_handler,                /* 12: Debug Monitor */
    0,                                          /* 13: Reserved */
    (uint32_t)pend_sv_handler,                  /* 14: PendSV */
    (uint32_t)sys_tick_handler,                 /* 15: SysTick */
    
    /* Peripheral interrupts (16-172) - omitted for brevity, use weak aliases */
    (uint32_t)wwdg_irq_handler,                 /* 16 */
    (uint32_t)pvd_irq_handler,                  /* 17 */
    (uint32_t)tamp_stamp_irq_handler,           /* 18 */
    (uint32_t)rtc_wkup_irq_handler,             /* 19 */
    (uint32_t)flash_irq_handler,                /* 20 */
    (uint32_t)rcc_irq_handler,                  /* 21 */
    (uint32_t)exti0_irq_handler,                /* 22 */
    (uint32_t)exti1_irq_handler,                /* 23 */
    (uint32_t)exti2_irq_handler,                /* 24 */
    (uint32_t)exti3_irq_handler,                /* 25 */
    (uint32_t)exti4_irq_handler,                /* 26 */
    (uint32_t)dma1_stream0_irq_handler,         /* 27 */
    (uint32_t)dma1_stream1_irq_handler,         /* 28 */
    (uint32_t)dma1_stream2_irq_handler,         /* 29 */
    (uint32_t)dma1_stream3_irq_handler,         /* 30 */
    (uint32_t)dma1_stream4_irq_handler,         /* 31 */
    (uint32_t)dma1_stream5_irq_handler,         /* 32 */
    (uint32_t)dma1_stream6_irq_handler,         /* 33 */
    (uint32_t)adc_irq_handler,                  /* 34 */
    (uint32_t)can1_tx_irq_handler,              /* 35 */
    (uint32_t)can1_rx0_irq_handler,             /* 36 */
    (uint32_t)can1_rx1_irq_handler,             /* 37 */
    (uint32_t)can1_sce_irq_handler,             /* 38 */
    (uint32_t)exti9_5_irq_handler,              /* 39 */
    (uint32_t)tim1_brk_tim9_irq_handler,        /* 40 */
    (uint32_t)tim1_up_tim10_irq_handler,        /* 41 */
    (uint32_t)tim1_trg_com_tim11_irq_handler,   /* 42 */
    (uint32_t)tim1_cc_irq_handler,              /* 43 */
    (uint32_t)tim2_irq_handler,                 /* 44 */
    (uint32_t)tim3_irq_handler,                 /* 45 */
    (uint32_t)tim4_irq_handler,                 /* 46 */
    (uint32_t)i2c1_ev_irq_handler,              /* 47 */
    (uint32_t)i2c1_er_irq_handler,              /* 48 */
    (uint32_t)i2c2_ev_irq_handler,              /* 49 */
    (uint32_t)i2c2_er_irq_handler,              /* 50 */
    (uint32_t)spi1_irq_handler,                 /* 51 */
    (uint32_t)spi2_irq_handler,                 /* 52 */
    (uint32_t)usart1_irq_handler,               /* 53 */
    (uint32_t)usart2_irq_handler,               /* 54 */
    (uint32_t)usart3_irq_handler,               /* 55 */
    (uint32_t)exti15_10_irq_handler,            /* 56 */
    (uint32_t)rtc_alarm_irq_handler,            /* 57 */
    (uint32_t)otg_fs_wkup_irq_handler,          /* 58 */
    (uint32_t)tim8_brk_tim12_irq_handler,       /* 59 */
    (uint32_t)tim8_up_tim13_irq_handler,        /* 60 */
    (uint32_t)tim8_trg_com_tim14_irq_handler,   /* 61 */
    (uint32_t)tim8_cc_irq_handler,              /* 62 */
    (uint32_t)dma1_stream7_irq_handler,         /* 63 */
    (uint32_t)fsmc_irq_handler,                 /* 64 */
    (uint32_t)sdio_irq_handler,                 /* 65 */
    (uint32_t)tim5_irq_handler,                 /* 66 */
    (uint32_t)spi3_irq_handler,                 /* 67 */
    (uint32_t)uart4_irq_handler,                /* 68 */
    (uint32_t)uart5_irq_handler,                /* 69 */
    (uint32_t)tim6_dac_irq_handler,             /* 70 */
    (uint32_t)tim7_irq_handler,                 /* 71 */
    (uint32_t)dma2_stream0_irq_handler,         /* 72 */
    (uint32_t)dma2_stream1_irq_handler,         /* 73 */
    (uint32_t)dma2_stream2_irq_handler,         /* 74 */
    (uint32_t)dma2_stream3_irq_handler,         /* 75 */
    (uint32_t)dma2_stream4_irq_handler,         /* 76 */
    (uint32_t)eth_irq_handler,                  /* 77 */
    (uint32_t)eth_wkup_irq_handler,             /* 78 */
    (uint32_t)can2_tx_irq_handler,              /* 79 */
    (uint32_t)can2_rx0_irq_handler,             /* 80 */
    (uint32_t)can2_rx1_irq_handler,             /* 81 */
    (uint32_t)can2_sce_irq_handler,             /* 82 */
    (uint32_t)otg_fs_irq_handler,               /* 83 */
    (uint32_t)dma2_stream5_irq_handler,         /* 84 */
    (uint32_t)dma2_stream6_irq_handler,         /* 85 */
    (uint32_t)dma2_stream7_irq_handler,         /* 86 */
    (uint32_t)usart6_irq_handler,               /* 87 */
    (uint32_t)i2c3_ev_irq_handler,              /* 88 */
    (uint32_t)i2c3_er_irq_handler,              /* 89 */
    (uint32_t)otg_hs_ep1_out_irq_handler,       /* 90 */
    (uint32_t)otg_hs_ep1_in_irq_handler,        /* 91 */
    (uint32_t)otg_hs_wkup_irq_handler,          /* 92 */
    (uint32_t)otg_hs_irq_handler,               /* 93 */
    (uint32_t)dcmi_irq_handler,                 /* 94 */
    (uint32_t)cryp_irq_handler,                 /* 95 */
    (uint32_t)hash_rng_irq_handler,             /* 96 */
    (uint32_t)fpu_irq_handler,                  /* 97 */
    (uint32_t)uart7_irq_handler,                /* 98 */
    (uint32_t)uart8_irq_handler,                /* 99 */
    (uint32_t)spi4_irq_handler,                 /* 100 */
    (uint32_t)spi5_irq_handler,                 /* 101 */
    (uint32_t)spi6_irq_handler,                 /* 102 */
    (uint32_t)sai1_irq_handler,                 /* 103 */
    (uint32_t)lcd_tft_irq_handler,              /* 104 */
    (uint32_t)lcd_tft_err_irq_handler,          /* 105 */
    (uint32_t)dma2d_irq_handler,                /* 106 */
};

/* ==================== RESET HANDLER ==================== */

/**
 * Reset handler - entry point for the application
 * Initializes global data, calls HAL init, scheduler init, then main
 */
void reset_handler(void) {
    /* Disable interrupts initially */
    __asm volatile("cpsid i");
    
    /* Initialize HAL (clock, FPU, interrupts) */
    hal_init();
    
    /* Initialize scheduler */
    scheduler_init();
    
    /* Call main application */
    main();
    
    /* Hang if main returns */
    while (1) {
        __asm volatile("wfi");  /* Wait for interrupt */
    }
}

/* ==================== STUB FUNCTIONS FOR LINKER ==================== */

/**
 * Weak symbols for data sections
 * Linker script will define _sidata, _sdata, _edata
 */
extern uint32_t _sidata, _sdata, _edata;
extern uint32_t _sbss, _ebss;
