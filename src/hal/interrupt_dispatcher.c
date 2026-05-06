/**
 * @file interrupt_dispatcher.c
 * @brief Generic Interrupt Dispatcher - Implementation
 * 
 * Implements a unified interrupt handling mechanism for all subsystems.
 * ZephyrOS parallel: This is similar to how ZephyrOS handles ISR registration.
 */

#include "hal/interrupt_dispatcher.h"
#include "kernel/memory.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* ==================== CONFIGURATION ==================== */

#define MAX_IRQ_HANDLERS    (256)  /* Support up to 256 IRQ lines */
#define INVALID_IRQ_NUM     (0xFF)

/* ==================== ISR CONTEXT TRACKING ==================== */

static volatile bool in_isr_context = false;

/* ==================== IRQ HANDLER TABLE ==================== */

typedef struct {
    irq_handler_t handler;      /* Handler function pointer */
    void *context;              /* User-provided context */
    bool registered;            /* Is this handler registered? */
} irq_entry_t;

static irq_entry_t irq_handlers[MAX_IRQ_HANDLERS];
static uint32_t handler_count = 0;

/* ==================== DISPATCHER INITIALIZATION ==================== */

void irq_dispatcher_init(void) {
    printf("[IRQ Dispatcher] Initializing...\n");
    
    /* Clear all handler entries */
    memset(irq_handlers, 0, sizeof(irq_handlers));
    handler_count = 0;
    in_isr_context = false;
    
    printf("[IRQ Dispatcher] Initialized: %d IRQ slots available\n", MAX_IRQ_HANDLERS);
}

/* ==================== HANDLER REGISTRATION ==================== */

rtos_status_t irq_register(uint8_t irq_num, irq_handler_t handler, void *context) {
    if (irq_num >= MAX_IRQ_HANDLERS) {
        printf("[IRQ Dispatcher] ERROR: IRQ %d out of range\n", irq_num);
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (handler == NULL) {
        printf("[IRQ Dispatcher] ERROR: NULL handler for IRQ %d\n", irq_num);
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* Check if already registered */
    if (irq_handlers[irq_num].registered) {
        printf("[IRQ Dispatcher] WARNING: IRQ %d already has a handler (overwriting)\n", irq_num);
    }
    
    /* Register the handler */
    irq_handlers[irq_num].handler = handler;
    irq_handlers[irq_num].context = context;
    irq_handlers[irq_num].registered = true;
    
    if (!irq_handlers[irq_num].registered) {
        handler_count++;
    }
    
    printf("[IRQ Dispatcher] Registered handler for IRQ %d (context: %p)\n", irq_num, context);
    
    return RTOS_OK;
}

rtos_status_t irq_unregister(uint8_t irq_num) {
    if (irq_num >= MAX_IRQ_HANDLERS) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!irq_handlers[irq_num].registered) {
        printf("[IRQ Dispatcher] ERROR: IRQ %d not registered\n", irq_num);
        return RTOS_ERROR;
    }
    
    irq_handlers[irq_num].handler = NULL;
    irq_handlers[irq_num].context = NULL;
    irq_handlers[irq_num].registered = false;
    handler_count--;
    
    printf("[IRQ Dispatcher] Unregistered handler for IRQ %d\n", irq_num);
    
    return RTOS_OK;
}

/* ==================== INTERRUPT DISPATCH ==================== */

rtos_status_t irq_dispatch(uint8_t irq_num) {
    /* Bounds check */
    if (irq_num >= MAX_IRQ_HANDLERS) {
        printf("[IRQ Dispatcher] ERROR: Invalid IRQ number %d\n", irq_num);
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* Mark that we're in ISR context */
    bool was_in_isr = in_isr_context;
    in_isr_context = true;
    
    /* Check if handler is registered */
    if (!irq_handlers[irq_num].registered) {
        printf("[IRQ Dispatcher] WARNING: No handler registered for IRQ %d\n", irq_num);
        in_isr_context = was_in_isr;
        return RTOS_ERROR;
    }
    
    /* Call the registered handler */
    irq_handler_t handler = irq_handlers[irq_num].handler;
    void *context = irq_handlers[irq_num].context;
    
    rtos_status_t status = handler(irq_num, context);
    
    /* Restore ISR context flag */
    in_isr_context = was_in_isr;
    
    return status;
}

/* ==================== GLOBAL INTERRUPT CONTROL ==================== */

/* Platform-specific implementations */

#ifdef TARGET_WINDOWS
/* Windows: No real interrupt control, just return dummy values */

void irq_enable_global(void) {
    /* No-op for Windows simulation */
}

uint32_t irq_disable_global(void) {
    /* Return dummy flags */
    return 0;
}

void irq_restore(uint32_t flags) {
    (void)flags;
    /* No-op for Windows simulation */
}

#endif /* TARGET_WINDOWS */

#ifdef TARGET_QEMU
/* ARM Cortex-M4: Use PRIMASK register */

void irq_enable_global(void) {
    __asm volatile("cpsie i");  /* Enable interrupts */
}

uint32_t irq_disable_global(void) {
    /* Read and save PRIMASK, then disable interrupts */
    uint32_t primask;
    __asm volatile("mrs %0, primask" : "=r" (primask));
    __asm volatile("cpsid i");
    return primask;
}

void irq_restore(uint32_t flags) {
    /* Restore PRIMASK to previous state */
    __asm volatile("msr primask, %0" : : "r" (flags));
}

#endif /* TARGET_QEMU */

/* ==================== ISR CONTEXT CHECKING ==================== */

bool irq_is_in_isr(void) {
    return in_isr_context;
}

/* ==================== DEBUG FUNCTIONS ==================== */

uint32_t irq_get_handler_count(void) {
    return handler_count;
}

void irq_print_handlers(void) {
    printf("\n=== Registered Interrupt Handlers ===\n");
    printf("Total registered handlers: %d\n", handler_count);
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_IRQ_HANDLERS; i++) {
        if (irq_handlers[i].registered) {
            printf("  IRQ %3d: Handler=%p, Context=%p\n", i, irq_handlers[i].handler, irq_handlers[i].context);
            count++;
        }
    }
    
    if (count == 0) {
        printf("  (none)\n");
    }
    
    printf("=====================================\n\n");
}
