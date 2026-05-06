/**
 * @file interrupt_dispatcher.h
 * @brief Generic Interrupt Dispatcher - Abstraction for ISR handling
 * 
 * Provides a subsystem-agnostic interrupt dispatch mechanism enabling
 * multiple drivers to register and handle interrupts without coupling.
 * 
 * ZephyrOS parallel: Mirrors ZephyrOS IRQ management where drivers
 * register handlers and the kernel provides a unified dispatcher.
 */

#ifndef INTERRUPT_DISPATCHER_H
#define INTERRUPT_DISPATCHER_H

#include "rtos_types.h"

/* ==================== TYPE DEFINITIONS ==================== */

/**
 * @brief IRQ handler callback type
 * 
 * Called when a registered interrupt fires.
 * 
 * @param irq_num The IRQ number that triggered the handler
 * @param context User-provided context data
 * @return Status of handler execution
 */
typedef rtos_status_t (*irq_handler_t)(uint8_t irq_num, void *context);

/* ==================== API FUNCTIONS ==================== */

/**
 * @brief Initialize the interrupt dispatcher
 * 
 * Must be called before registering any interrupt handlers.
 * Sets up the handler table and related structures.
 */
void irq_dispatcher_init(void);

/**
 * @brief Register an interrupt handler for a specific IRQ
 * 
 * @param irq_num IRQ number (platform-specific, e.g., 0-239 for Cortex-M4)
 * @param handler Function to call when IRQ fires
 * @param context User-provided context passed to handler
 * @return RTOS_OK on success, RTOS_ERROR if registration fails
 * 
 * ZephyrOS parallel: irq_connect_dynamic() or irq_connect()
 * 
 * Example (UART interrupt):
 *   irq_register(UART0_IRQ_NUM, uart_irq_handler, (void *)uart_handle);
 */
rtos_status_t irq_register(uint8_t irq_num, irq_handler_t handler, void *context);

/**
 * @brief Unregister a previously registered interrupt handler
 * 
 * @param irq_num IRQ number to unregister
 * @return RTOS_OK on success, RTOS_ERROR_NOT_FOUND if not registered
 */
rtos_status_t irq_unregister(uint8_t irq_num);

/**
 * @brief Dispatch an interrupt to its registered handler
 * 
 * Called by the ISR vector table when an interrupt fires.
 * This is the central dispatcher that finds and calls the handler.
 * 
 * @param irq_num IRQ number that fired
 * @return RTOS_OK if handler executed successfully
 * 
 * ZephyrOS parallel: _isr_wrapper or similar mechanism
 * 
 * Note: This function is typically called from hardware ISR context.
 */
rtos_status_t irq_dispatch(uint8_t irq_num);

/**
 * @brief Enable interrupts globally
 * 
 * Enables CPU interrupt line (PRIMASK/BASEPRI on Cortex-M).
 * ZephyrOS parallel: irq_unlock() / irq_enable()
 */
void irq_enable_global(void);

/**
 * @brief Disable interrupts globally
 * 
 * Disables CPU interrupt line to protect critical sections.
 * ZephyrOS parallel: irq_lock() / irq_disable()
 */
uint32_t irq_disable_global(void);

/**
 * @brief Restore previous interrupt state
 * 
 * Restores the CPU interrupt state to a previously saved value.
 * Used with irq_disable_global() for critical sections.
 * 
 * ZephyrOS parallel: irq_unlock(flags)
 */
void irq_restore(uint32_t flags);

/**
 * @brief Check if currently in interrupt context
 * 
 * @return true if currently executing in an ISR context, false otherwise
 * 
 * ZephyrOS parallel: k_is_in_isr()
 */
bool irq_is_in_isr(void);

/**
 * @brief Get IRQ handler count (debug function)
 * 
 * @return Number of registered IRQ handlers
 */
uint32_t irq_get_handler_count(void);

/**
 * @brief Print interrupt dispatcher status (debug function)
 * 
 * Enumerates all registered handlers and their IRQ numbers.
 */
void irq_print_handlers(void);

#endif /* INTERRUPT_DISPATCHER_H */
