#include "kernel/scheduler.h"
#include "kernel/task.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Ready lists - one per priority level */
static tcb_t *ready_lists[NUM_PRIORITY_LEVELS];
static volatile tick_t system_tick_count = 0;
static bool scheduler_running = false;
static tcb_t *current_tcb = NULL;

/* Delayed task list */
static tcb_t *delayed_list = NULL;

/* Initialize scheduler */
void scheduler_init(void) {
    memset(ready_lists, 0, sizeof(ready_lists));
    system_tick_count = 0;
    scheduler_running = false;
    current_tcb = NULL;
    delayed_list = NULL;
    
    printf("[SCHEDULER] Initialized\n");
}

/* Add task to ready list */
void scheduler_add_task(tcb_t *tcb) {
    if (tcb == NULL || tcb->priority >= NUM_PRIORITY_LEVELS) {
        return;
    }
    
    tcb->state = TASK_STATE_READY;
    tcb->next = NULL;
    
    /* Add to end of priority queue (round-robin for same priority) */
    if (ready_lists[tcb->priority] == NULL) {
        ready_lists[tcb->priority] = tcb;
    } else {
        tcb_t *tail = ready_lists[tcb->priority];
        while (tail->next != NULL) {
            tail = tail->next;
        }
        tail->next = tcb;
    }
    
    printf("[SCHEDULER] Added task '%s' to ready list (Priority: %u)\n", 
           tcb->name, tcb->priority);
}

/* Remove task from ready list */
void scheduler_remove_task(tcb_t *tcb) {
    if (tcb == NULL || tcb->priority >= NUM_PRIORITY_LEVELS) {
        return;
    }
    
    tcb_t **list = &ready_lists[tcb->priority];
    tcb_t *prev = NULL;
    tcb_t *curr = *list;
    
    while (curr != NULL) {
        if (curr == tcb) {
            if (prev == NULL) {
                *list = curr->next;
            } else {
                prev->next = curr->next;
            }
            tcb->next = NULL;
            printf("[SCHEDULER] Removed task '%s' from ready list\n", tcb->name);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

/* Get highest priority ready task
   0 being highest priority, and larger number are lower priority */
static tcb_t* get_highest_priority_task(void) {
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        if (ready_lists[i] != NULL) {
            return ready_lists[i];
        }
    }
    return NULL;
}

/* Perform scheduling decision */
static void schedule(void) {
    tcb_t *next_task = get_highest_priority_task();
    
    if (next_task == NULL) {
        printf("[SCHEDULER] No ready tasks!\n");
        return;
    }
    
    /* If task changed, perform context switch */
    if (next_task != current_tcb) {
        tcb_t *prev_task = current_tcb;
        
        /* Rotate current task to end of its priority queue if still ready */
        if (current_tcb && current_tcb->state == TASK_STATE_READY) {
            scheduler_remove_task(current_tcb);
            scheduler_add_task(current_tcb);
        }
        
        /* Remove next task from ready list and mark as running */
        scheduler_remove_task(next_task);
        next_task->state = TASK_STATE_RUNNING;
        current_tcb = next_task;
        task_set_current(current_tcb);
        
        printf("[SCHEDULER] Context switch: %s -> %s\n",
               prev_task ? prev_task->name : "NULL",
               next_task->name);
        
        /* Perform actual context switch */
        if (prev_task) {
            context_switch(prev_task, next_task);
        }
    }
}

/* Yield CPU */
void scheduler_yield(void) {
    if (!scheduler_running) {
        return;
    }
    
    RTOS_ENTER_CRITICAL();
    
    /* Mark current task as ready if it was running */
    if (current_tcb && current_tcb->state == TASK_STATE_RUNNING) {
        current_tcb->state = TASK_STATE_READY;
        scheduler_add_task(current_tcb);
    }
    
    schedule();
    
    RTOS_EXIT_CRITICAL();
}

/* Add task to delayed list */
static void add_to_delayed_list(tcb_t *task) {
    task->next = NULL;
    
    if (delayed_list == NULL) {
        delayed_list = task;
    } else {
        /* Insert sorted by wake time */
        if (task->wake_time < delayed_list->wake_time) {
            task->next = delayed_list;
            delayed_list = task;
        } else {
            tcb_t *curr = delayed_list;
            while (curr->next != NULL && curr->next->wake_time <= task->wake_time) {
                curr = curr->next;
            }
            task->next = curr->next;
            curr->next = task;
        }
    }
}

/* Process delayed tasks */
static void process_delayed_tasks(void) {
    while (delayed_list != NULL && delayed_list->wake_time <= system_tick_count) {
        tcb_t *task = delayed_list;
        delayed_list = task->next;
        
        /* Add to ready list */
        task->state = TASK_STATE_READY;
        task->next = NULL;
        scheduler_add_task(task);
        
        printf("[SCHEDULER] Task '%s' awakened at tick %u\n", 
               task->name, (unsigned int)system_tick_count);
    }
}

/* System tick handler */
void scheduler_tick(void) {
    system_tick_count++;
    
    /* Process delayed tasks */
    process_delayed_tasks();
    
    /* Check for preemption */
    if (scheduler_running) {
        tcb_t *highest = get_highest_priority_task();
        if (highest && current_tcb && 
            highest->priority < current_tcb->priority) {
            /* Higher priority task ready - preempt */
            printf("[SCHEDULER] Preemption at tick %u\n", (unsigned int)system_tick_count);
            scheduler_yield();
        }
    }
}

/* Get current tick count */
tick_t scheduler_get_tick_count(void) {
    return system_tick_count;
}

/* Start scheduler */
void scheduler_start(void) {
    printf("[SCHEDULER] Starting...\n");
    scheduler_running = true;
    schedule();
}

/* Context switch implementation (simulated for x86/Windows) */
void context_switch(tcb_t *current, tcb_t *next) {
    /* 
     * ARM Cortex-M Context Switch Reference
     * ======================================
     * 
     * In real ARM implementation, this would use PendSV interrupt handler:
     * 
     * void PendSV_Handler(void) {
     *     __asm volatile (
     *         "CPSID   I                      \n"  // Disable interrupts
     *         "MRS     R0, PSP                \n"  // Get Process Stack Pointer
     *         "CBZ     R0, restore_ctx        \n"  // Skip save if first switch
     *         "STMDB   R0!, {R4-R11}          \n"  // Save R4-R11 to stack
     *         "LDR     R1, =current_tcb       \n"  // Get current TCB address
     *         "LDR     R1, [R1]               \n"  // Load current TCB
     *         "STR     R0, [R1]               \n"  // Save stack pointer to TCB
     *         "restore_ctx:                   \n"
     *         "LDR     R1, =next_tcb          \n"  // Get next TCB address
     *         "LDR     R1, [R1]               \n"  // Load next TCB
     *         "LDR     R0, [R1]               \n"  // Load stack pointer from TCB
     *         "LDMIA   R0!, {R4-R11}          \n"  // Restore R4-R11 from stack
     *         "MSR     PSP, R0                \n"  // Set new stack pointer
     *         "CPSIE   I                      \n"  // Enable interrupts
     *         "BX      LR                     \n"  // Return (auto-restore R0-R3,R12,LR,PC,xPSR)
     *     );
     * }
     * 
     * Stack Frame Layout:
     * Hardware auto-saves (exception entry): R0-R3, R12, LR, PC, xPSR (8 registers)
     * Software manual-saves: R4-R11 (8 registers)
     * Total context: 16 registers × 4 bytes = 64 bytes per task
     */
    
    /* Windows/x86 simulation */
    (void)current;
    (void)next;
    
    printf("[CONTEXT_SWITCH] %s -> %s (simulated)\n",
           current ? current->name : "NULL",
           next ? next->name : "NULL");
    
    #ifdef _WIN32
    /* Simulate ~240ns context switch time with 1ms delay for visibility */
    Sleep(1);
    #endif
}