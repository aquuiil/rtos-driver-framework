#include "ipc/semaphore.h"
#include "kernel/memory.h"
#include <stdio.h>

struct semaphore {
    uint32_t count;
    uint32_t max_count;
};

semaphore_t* sem_create(uint32_t initial_count, uint32_t max_count) {
    semaphore_t *sem = rtos_malloc(sizeof(semaphore_t));
    if (!sem) return NULL;
    
    sem->count = initial_count;
    sem->max_count = max_count;
    
    printf("[SEM] Created: count=%u, max=%u\n", initial_count, max_count);
    return sem;
}

rtos_status_t sem_take(semaphore_t *sem, tick_t timeout) {
    (void)timeout;
    if (!sem) return RTOS_ERROR_INVALID_PARAM;
    if (sem->count == 0) return RTOS_ERROR_TIMEOUT;
    
    sem->count--;
    printf("[SEM] Take (count=%u)\n", sem->count);
    return RTOS_OK;
}

rtos_status_t sem_give(semaphore_t *sem) {
    if (!sem) return RTOS_ERROR_INVALID_PARAM;
    if (sem->count >= sem->max_count) return RTOS_ERROR;
    
    sem->count++;
    printf("[SEM] Give (count=%u)\n", sem->count);
    return RTOS_OK;
}

void sem_destroy(semaphore_t *sem) {
    if (sem) {
        rtos_free(sem);
        printf("[SEM] Destroyed\n");
    }
}