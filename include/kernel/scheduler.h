#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "rtos_types.h"

/* Forward declaration */
typedef struct task_control_block tcb_t;

/* Scheduler API */
void scheduler_init(void);
void scheduler_start(void);
void scheduler_add_task(tcb_t *tcb);
void scheduler_remove_task(tcb_t *tcb);
void scheduler_yield(void);
void scheduler_tick(void);
tick_t scheduler_get_tick_count(void);

/* Context switching (platform specific) */
void context_switch(tcb_t *current, tcb_t *next);

#endif /* SCHEDULER_H */