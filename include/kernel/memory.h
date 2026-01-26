#ifndef MEMORY_H
#define MEMORY_H

#include "rtos_types.h"

/* Memory block header for heap allocator */
typedef struct memory_block {
    size_t size;                    // Size of this block (excluding header)
    bool is_free;                   // Free/allocated flag
    struct memory_block *next;      // Next block in list
    uint32_t magic;                 // Magic number for corruption detection
} memory_block_t;

/* Memory pool for fixed-size allocations */
typedef struct {
    void *pool_start;               // Start of memory pool
    size_t block_size;              // Size of each block
    size_t num_blocks;              // Total number of blocks
    size_t free_blocks;             // Number of free blocks
    uint8_t *free_list;             // Bitmap of free blocks
} memory_pool_t;

/* Memory statistics */
typedef struct {
    size_t total_heap_size;         // Total heap size
    size_t used_heap_size;          // Currently used heap
    size_t free_heap_size;          // Currently free heap
    size_t largest_free_block;      // Largest contiguous free block
    uint32_t num_allocations;       // Total allocations
    uint32_t num_frees;             // Total frees
    uint32_t fragmentation_percent; // Heap fragmentation percentage
} memory_stats_t;

/* Memory API */
void memory_init(void);
void* rtos_malloc(size_t size);
void rtos_free(void *ptr);
void* rtos_calloc(size_t num, size_t size);
void* rtos_realloc(void *ptr, size_t new_size);

/* Memory pool API */
rtos_status_t memory_pool_create(
    memory_pool_t *pool,
    size_t block_size,
    size_t num_blocks
);
void* memory_pool_alloc(memory_pool_t *pool);
rtos_status_t memory_pool_free(memory_pool_t *pool, void *ptr);
void memory_pool_destroy(memory_pool_t *pool);

/* Memory statistics */
void memory_get_stats(memory_stats_t *stats);
void memory_print_stats(void);

/* Stack overflow detection */
bool memory_check_stack_overflow(uint32_t task_id);

#endif /* MEMORY_H */