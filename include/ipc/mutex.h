#ifndef MUTEX_H
#define MUTEX_H

#include "rtos_types.h"

typedef struct mutex mutex_t;

mutex_t* mutex_create(void);
rtos_status_t mutex_lock(mutex_t *mutex, tick_t timeout);
rtos_status_t mutex_unlock(mutex_t *mutex);
void mutex_destroy(mutex_t *mutex);

#endif