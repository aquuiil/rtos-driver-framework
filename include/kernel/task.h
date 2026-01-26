#ifndef TASK_H
#define TASK_H

#include "rtos_types.h"

/* Task Control Block (TCB) */
typedef struct task_control_block {
    uint32_t *stack_ptr;              // Current stack pointer
    uint32_t stack[TASK_STACK_SIZE];  // Task stack
    task_state_t state;                // Current task state
    uint8_t priority;                  // Task priority (0-31)
    uint32_t task_id;                  // Unique task ID
    char name[16];                     // Task name for debugging
    tick_t wake_time;                  // Wake time for delayed tasks
    struct task_control_block *next;   // Next task in ready list
} tcb_t;

/* Task function pointer type */
typedef void (*task_function_t)(void *params);

/* Task API */
rtos_status_t task_create(
    task_function_t function,
    const char *name,
    uint8_t priority,
    void *params,
    uint32_t *task_id
);

rtos_status_t task_delete(uint32_t task_id);
rtos_status_t task_suspend(uint32_t task_id);
rtos_status_t task_resume(uint32_t task_id);
void task_delay(tick_t ticks);
void task_yield(void);
uint32_t task_get_current_id(void);
tcb_t* task_get_current_tcb(void);

/* Internal functions */
void task_init(void);
tcb_t* task_get_tcb(uint32_t task_id);
void task_set_current(tcb_t *tcb);

#endif /* TASK_H */