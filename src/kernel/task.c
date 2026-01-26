#include "kernel/task.h"
#include "kernel/scheduler.h"
#include "kernel/memory.h"
#include <string.h>
#include <stdio.h>

/* Task pool */
static tcb_t task_pool[MAX_TASKS];
static uint32_t next_task_id = 1;
static tcb_t *current_task = NULL;

/* Initialize task management system */
void task_init(void) {
    memset(task_pool, 0, sizeof(task_pool));
    next_task_id = 1;
    current_task = NULL;
    
    printf("[TASK] Task management initialized\n");
}

/* Stack initialization for context switching */
static void task_init_stack(tcb_t *tcb, task_function_t function, void *params) {
    uint32_t *stack_top = &tcb->stack[TASK_STACK_SIZE - 1];
    
    /* 
     * Simulating ARM Cortex-M stack frame for educational purposes
     * In real ARM Cortex-M, the hardware automatically saves/restores:
     * R0, R1, R2, R3, R12, LR, PC, xPSR (8 registers)
     * 
     * Software must save/restore: R4-R11 (8 registers)
     * 
     * Stack grows downward (high address to low address)
     */
    
    /* Hardware-stacked registers (automatically saved by ARM on interrupt) */
    *(--stack_top) = 0x01000000;  // xPSR (Thumb bit set)
    *(--stack_top) = (uint32_t)function;  // PC (Program Counter)
    *(--stack_top) = 0xFFFFFFFD;  // LR (Link Register - EXC_RETURN value)
    *(--stack_top) = 0x12121212;  // R12
    *(--stack_top) = 0x03030303;  // R3
    *(--stack_top) = 0x02020202;  // R2
    *(--stack_top) = 0x01010101;  // R1
    *(--stack_top) = (uint32_t)params;  // R0 (function parameter)
    
    /* Software-stacked registers (manually saved in context switch) */
    *(--stack_top) = 0x11111111;  // R11
    *(--stack_top) = 0x10101010;  // R10
    *(--stack_top) = 0x09090909;  // R9
    *(--stack_top) = 0x08080808;  // R8
    *(--stack_top) = 0x07070707;  // R7
    *(--stack_top) = 0x06060606;  // R6
    *(--stack_top) = 0x05050505;  // R5
    *(--stack_top) = 0x04040404;  // R4
    
    tcb->stack_ptr = stack_top;
}

/* Create a new task */
rtos_status_t task_create(
    task_function_t function,
    const char *name,
    uint8_t priority,
    void *params,
    uint32_t *task_id
) {
    if (function == NULL || priority >= NUM_PRIORITY_LEVELS) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    RTOS_ENTER_CRITICAL();
    
    /* Find free TCB */
    tcb_t *tcb = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].state == TASK_STATE_DELETED || 
            task_pool[i].task_id == 0) {
            tcb = &task_pool[i];
            break;
        }
    }
    
    if (tcb == NULL) {
        RTOS_EXIT_CRITICAL();
        return RTOS_ERROR_NO_MEMORY;
    }
    
    /* Initialize TCB */
    memset(tcb, 0, sizeof(tcb_t));
    tcb->task_id = next_task_id++;
    tcb->priority = priority;
    tcb->state = TASK_STATE_READY;
    strncpy(tcb->name, name ? name : "unnamed", sizeof(tcb->name) - 1);
    tcb->name[sizeof(tcb->name) - 1] = '\0';  // Ensure null termination
    
    /* Initialize stack */
    task_init_stack(tcb, function, params);
    
    /* Add to scheduler */
    scheduler_add_task(tcb);
    
    if (task_id) {
        *task_id = tcb->task_id;
    }
    
    RTOS_EXIT_CRITICAL();
    
    printf("[TASK] Created task '%s' (ID: %u, Priority: %u)\n", 
           tcb->name, tcb->task_id, tcb->priority);
    
    return RTOS_OK;
}

/* Delete a task */
rtos_status_t task_delete(uint32_t task_id) {
    RTOS_ENTER_CRITICAL();
    
    tcb_t *tcb = task_get_tcb(task_id);
    if (tcb == NULL) {
        RTOS_EXIT_CRITICAL();
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* Remove from scheduler */
    scheduler_remove_task(tcb);
    
    tcb->state = TASK_STATE_DELETED;
    
    RTOS_EXIT_CRITICAL();
    
    printf("[TASK] Deleted task '%s' (ID: %u)\n", tcb->name, tcb->task_id);
    
    /* If deleting current task, trigger scheduling */
    if (tcb == current_task) {
        task_yield();
    }
    
    return RTOS_OK;
}

/* Suspend a task */
rtos_status_t task_suspend(uint32_t task_id) {
    RTOS_ENTER_CRITICAL();
    
    tcb_t *tcb = task_get_tcb(task_id);
    if (tcb == NULL || tcb->state == TASK_STATE_DELETED) {
        RTOS_EXIT_CRITICAL();
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    tcb->state = TASK_STATE_SUSPENDED;
    scheduler_remove_task(tcb);
    
    RTOS_EXIT_CRITICAL();
    
    printf("[TASK] Suspended task '%s' (ID: %u)\n", tcb->name, tcb->task_id);
    
    if (tcb == current_task) {
        task_yield();
    }
    
    return RTOS_OK;
}

/* Resume a suspended task */
rtos_status_t task_resume(uint32_t task_id) {
    RTOS_ENTER_CRITICAL();
    
    tcb_t *tcb = task_get_tcb(task_id);
    if (tcb == NULL || tcb->state != TASK_STATE_SUSPENDED) {
        RTOS_EXIT_CRITICAL();
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    tcb->state = TASK_STATE_READY;
    scheduler_add_task(tcb);
    
    RTOS_EXIT_CRITICAL();
    
    printf("[TASK] Resumed task '%s' (ID: %u)\n", tcb->name, tcb->task_id);
    
    return RTOS_OK;
}

/* Delay current task */
void task_delay(tick_t ticks) {
    if (current_task && ticks > 0) {
        RTOS_ENTER_CRITICAL();
        current_task->wake_time = scheduler_get_tick_count() + ticks;
        current_task->state = TASK_STATE_BLOCKED;
        scheduler_remove_task(current_task);
        RTOS_EXIT_CRITICAL();
        task_yield();
    }
}

/* Yield CPU to another task */
void task_yield(void) {
    scheduler_yield();
}

/* Get current task ID */
uint32_t task_get_current_id(void) {
    return current_task ? current_task->task_id : 0;
}

/* Get current TCB */
tcb_t* task_get_current_tcb(void) {
    return current_task;
}

/* Get TCB by task ID */
tcb_t* task_get_tcb(uint32_t task_id) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_pool[i].task_id == task_id && 
            task_pool[i].state != TASK_STATE_DELETED) {
            return &task_pool[i];
        }
    }
    return NULL;
}

/* Set current task (called by scheduler) */
void task_set_current(tcb_t *tcb) {
    current_task = tcb;
}