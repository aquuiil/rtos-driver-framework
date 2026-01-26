#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "rtos_types.h"

typedef struct semaphore semaphore_t;

semaphore_t* sem_create(uint32_t initial_count, uint32_t max_count);
rtos_status_t sem_take(semaphore_t *sem, tick_t timeout);
rtos_status_t sem_give(semaphore_t *sem);
void sem_destroy(semaphore_t *sem);

#endif