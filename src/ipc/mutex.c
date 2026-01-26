#include "ipc/mutex.h"
#include "kernel/memory.h"
#include "kernel/task.h"
#include <stdio.h>

struct mutex {
    bool locked;
    uint32_t owner_task_id;
};

mutex_t* mutex_create(void) {
    mutex_t *mutex = rtos_malloc(sizeof(mutex_t));
    if (!mutex) return NULL;
    
    mutex->locked = false;
    mutex->owner_task_id = 0;
    
    printf("[MUTEX] Created\n");
    return mutex;
}

rtos_status_t mutex_lock(mutex_t *mutex, tick_t timeout) {
    (void)timeout;
    if (!mutex) return RTOS_ERROR_INVALID_PARAM;
    
    if (mutex->locked) {
        printf("[MUTEX] Already locked by task %u\n", mutex->owner_task_id);
        return RTOS_ERROR_TIMEOUT;
    }
    
    mutex->locked = true;
    mutex->owner_task_id = task_get_current_id();
    
    printf("[MUTEX] Locked by task %u\n", mutex->owner_task_id);
    return RTOS_OK;
}

rtos_status_t mutex_unlock(mutex_t *mutex) {
    if (!mutex) return RTOS_ERROR_INVALID_PARAM;
    if (!mutex->locked) return RTOS_ERROR;
    
    uint32_t current_task = task_get_current_id();
    if (mutex->owner_task_id != current_task) {
        printf("[MUTEX] ERROR: Task %u trying to unlock mutex owned by %u\n",
               current_task, mutex->owner_task_id);
        return RTOS_ERROR;
    }
    
    mutex->locked = false;
    mutex->owner_task_id = 0;
    
    printf("[MUTEX] Unlocked by task %u\n", current_task);
    return RTOS_OK;
}

void mutex_destroy(mutex_t *mutex) {
    if (mutex) {
        rtos_free(mutex);
        printf("[MUTEX] Destroyed\n");
    }
}