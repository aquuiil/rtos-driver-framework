#ifndef RTOS_TYPES_H
#define RTOS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Windows compatibility */
#ifdef _WIN32
    #define RTOS_PACKED __attribute__((packed))
#else
    #define RTOS_PACKED __attribute__((packed))
#endif

/* Status codes */
typedef enum {
    RTOS_OK = 0,
    RTOS_ERROR = -1,
    RTOS_ERROR_TIMEOUT = -2,
    RTOS_ERROR_NO_MEMORY = -3,
    RTOS_ERROR_INVALID_PARAM = -4,
    RTOS_ERROR_BUSY = -5,
    RTOS_ERROR_NOT_READY = -6
} rtos_status_t;

/* Task states */
typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED,
    TASK_STATE_DELETED
} task_state_t;

/* Task priorities (0 = highest, 31 = lowest) */
#define TASK_PRIORITY_HIGHEST   0
#define TASK_PRIORITY_HIGH      8
#define TASK_PRIORITY_NORMAL    16
#define TASK_PRIORITY_LOW       24
#define TASK_PRIORITY_IDLE      31
#define NUM_PRIORITY_LEVELS     32

/* Configuration */
#define MAX_TASKS               16
#define TASK_STACK_SIZE         1024
#define HEAP_SIZE              (64 * 1024)  // 64KB heap

/* Time definitions */
typedef uint32_t tick_t;
#define TICK_RATE_HZ           1000  // 1ms tick
#define MS_TO_TICKS(ms)        ((ms) * TICK_RATE_HZ / 1000)
#define RTOS_WAIT_FOREVER      0xFFFFFFFF

/* Thread safety (for Windows simulation) */
#ifdef _WIN32
    #include <windows.h>
    typedef CRITICAL_SECTION rtos_mutex_handle_t;
    #define RTOS_ENTER_CRITICAL()  // Simulated - real embedded would disable interrupts
    #define RTOS_EXIT_CRITICAL()   // Simulated - real embedded would enable interrupts
#else
    #define RTOS_ENTER_CRITICAL()
    #define RTOS_EXIT_CRITICAL()
#endif

#endif /* RTOS_TYPES_H */