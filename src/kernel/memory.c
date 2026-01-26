#include "kernel/memory.h"
#include "kernel/task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Magic number for corruption detection */
#define MEMORY_MAGIC 0xDEADBEEF
#define STACK_CANARY 0xCAFEBABE

/* Heap memory */
static uint8_t heap_memory[HEAP_SIZE];
static memory_block_t *heap_head = NULL;
static bool memory_initialized = false;

/* Statistics */
static memory_stats_t global_stats = {0};

/* Initialize memory management system */
void memory_init(void) {
    if (memory_initialized) {
        return;
    }
    
    /* Initialize heap with single free block */
    heap_head = (memory_block_t*)heap_memory;
    heap_head->size = HEAP_SIZE - sizeof(memory_block_t);
    heap_head->is_free = true;
    heap_head->next = NULL;
    heap_head->magic = MEMORY_MAGIC;
    
    /* Initialize statistics */
    global_stats.total_heap_size = HEAP_SIZE;
    global_stats.used_heap_size = sizeof(memory_block_t);
    global_stats.free_heap_size = heap_head->size;
    global_stats.largest_free_block = heap_head->size;
    global_stats.num_allocations = 0;
    global_stats.num_frees = 0;
    global_stats.fragmentation_percent = 0;
    
    memory_initialized = true;
    
    printf("[MEMORY] Initialized heap (%u bytes)\n", HEAP_SIZE);
}

/* Align size to 4-byte boundary */
static size_t align_size(size_t size) {
    return (size + 3) & ~3;
}

/* Find best fit free block */
static memory_block_t* find_best_fit(size_t size) {
    memory_block_t *current = heap_head;
    memory_block_t *best_fit = NULL;
    size_t min_diff = SIZE_MAX;
    
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            size_t diff = current->size - size;
            if (diff < min_diff) {
                min_diff = diff;
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    return best_fit;
}

/* Split block if large enough */
static void split_block(memory_block_t *block, size_t size) {
    /* Only split if remaining size is significant */
    if (block->size >= size + sizeof(memory_block_t) + 32) {
        memory_block_t *new_block = (memory_block_t*)((uint8_t*)block + 
                                     sizeof(memory_block_t) + size);
        new_block->size = block->size - size - sizeof(memory_block_t);
        new_block->is_free = true;
        new_block->next = block->next;
        new_block->magic = MEMORY_MAGIC;
        
        block->size = size;
        block->next = new_block;
    }
}

/* Merge adjacent free blocks */
static void merge_free_blocks(void) {
    memory_block_t *current = heap_head;
    
    while (current != NULL && current->next != NULL) {
        if (current->is_free && current->next->is_free) {
            /* Merge with next block */
            current->size += sizeof(memory_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}

/* Allocate memory from heap */
void* rtos_malloc(size_t size) {
    if (!memory_initialized || size == 0) {
        return NULL;
    }
    
    RTOS_ENTER_CRITICAL();
    
    /* Align size */
    size = align_size(size);
    
    /* Find best fit block */
    memory_block_t *block = find_best_fit(size);
    if (block == NULL) {
        RTOS_EXIT_CRITICAL();
        printf("[MEMORY] Allocation failed: No block large enough for %zu bytes\n", size);
        return NULL;
    }
    
    /* Split block if possible */
    split_block(block, size);
    
    /* Mark block as used */
    block->is_free = false;
    
    /* Update statistics */
    global_stats.num_allocations++;
    global_stats.used_heap_size += block->size + sizeof(memory_block_t);
    global_stats.free_heap_size -= block->size + sizeof(memory_block_t);
    
    RTOS_EXIT_CRITICAL();
    
    /* Return pointer to data (skip header) */
    void *ptr = (void*)((uint8_t*)block + sizeof(memory_block_t));
    
    printf("[MEMORY] Allocated %zu bytes at %p\n", size, ptr);
    
    return ptr;
}

/* Free allocated memory */
void rtos_free(void *ptr) {
    if (!memory_initialized || ptr == NULL) {
        return;
    }
    
    RTOS_ENTER_CRITICAL();
    
    /* Get block header */
    memory_block_t *block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    
    /* Verify magic number */
    if (block->magic != MEMORY_MAGIC) {
        RTOS_EXIT_CRITICAL();
        printf("[MEMORY] ERROR: Heap corruption detected at %p!\n", ptr);
        return;
    }
    
    /* Check if already free */
    if (block->is_free) {
        RTOS_EXIT_CRITICAL();
        printf("[MEMORY] WARNING: Double free detected at %p!\n", ptr);
        return;
    }
    
    /* Mark block as free */
    block->is_free = true;
    
    /* Update statistics */
    global_stats.num_frees++;
    global_stats.used_heap_size -= block->size + sizeof(memory_block_t);
    global_stats.free_heap_size += block->size + sizeof(memory_block_t);
    
    /* Merge adjacent free blocks */
    merge_free_blocks();
    
    RTOS_EXIT_CRITICAL();
    
    printf("[MEMORY] Freed %zu bytes at %p\n", block->size, ptr);
}

/* Allocate and zero-initialize memory */
void* rtos_calloc(size_t num, size_t size) {
    size_t total_size = num * size;
    void *ptr = rtos_malloc(total_size);
    
    if (ptr != NULL) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

/* Reallocate memory */
void* rtos_realloc(void *ptr, size_t new_size) {
    if (ptr == NULL) {
        return rtos_malloc(new_size);
    }
    
    if (new_size == 0) {
        rtos_free(ptr);
        return NULL;
    }
    
    /* Get current block */
    memory_block_t *block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
    
    /* If new size fits in current block, just return same pointer */
    if (new_size <= block->size) {
        return ptr;
    }
    
    /* Allocate new block */
    void *new_ptr = rtos_malloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    /* Copy old data */
    memcpy(new_ptr, ptr, block->size);
    
    /* Free old block */
    rtos_free(ptr);
    
    return new_ptr;
}

/* Create memory pool */
rtos_status_t memory_pool_create(
    memory_pool_t *pool,
    size_t block_size,
    size_t num_blocks
) {
    if (pool == NULL || block_size == 0 || num_blocks == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* Align block size */
    block_size = align_size(block_size);
    
    /* Allocate pool memory */
    size_t total_size = block_size * num_blocks;
    pool->pool_start = rtos_malloc(total_size);
    if (pool->pool_start == NULL) {
        return RTOS_ERROR_NO_MEMORY;
    }
    
    /* Allocate free list bitmap (1 bit per block) */
    size_t bitmap_size = (num_blocks + 7) / 8;
    pool->free_list = (uint8_t*)rtos_malloc(bitmap_size);
    if (pool->free_list == NULL) {
        rtos_free(pool->pool_start);
        return RTOS_ERROR_NO_MEMORY;
    }
    
    /* Initialize pool */
    pool->block_size = block_size;
    pool->num_blocks = num_blocks;
    pool->free_blocks = num_blocks;
    memset(pool->free_list, 0xFF, bitmap_size); // All blocks free (1=free)
    
    printf("[MEMORY] Created pool: %zu blocks x %zu bytes = %zu total\n",
           num_blocks, block_size, total_size);
    
    return RTOS_OK;
}

/* Allocate block from pool */
void* memory_pool_alloc(memory_pool_t *pool) {
    if (pool == NULL || pool->free_blocks == 0) {
        return NULL;
    }
    
    RTOS_ENTER_CRITICAL();
    
    /* Find first free block */
    for (size_t i = 0; i < pool->num_blocks; i++) {
        size_t byte_idx = i / 8;
        size_t bit_idx = i % 8;
        
        if (pool->free_list[byte_idx] & (1 << bit_idx)) {
            /* Mark block as used */
            pool->free_list[byte_idx] &= ~(1 << bit_idx);
            pool->free_blocks--;
            
            RTOS_EXIT_CRITICAL();
            
            /* Calculate block address */
            void *ptr = (uint8_t*)pool->pool_start + (i * pool->block_size);
            
            printf("[MEMORY] Pool allocated block %zu at %p\n", i, ptr);
            
            return ptr;
        }
    }
    
    RTOS_EXIT_CRITICAL();
    return NULL;
}

/* Free block back to pool */
rtos_status_t memory_pool_free(memory_pool_t *pool, void *ptr) {
    if (pool == NULL || ptr == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /* Calculate block index */
    size_t offset = (uint8_t*)ptr - (uint8_t*)pool->pool_start;
    if (offset % pool->block_size != 0) {
        printf("[MEMORY] ERROR: Invalid pool pointer %p\n", ptr);
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    size_t block_idx = offset / pool->block_size;
    if (block_idx >= pool->num_blocks) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    RTOS_ENTER_CRITICAL();
    
    size_t byte_idx = block_idx / 8;
    size_t bit_idx = block_idx % 8;
    
    /* Check if already free */
    if (pool->free_list[byte_idx] & (1 << bit_idx)) {
        RTOS_EXIT_CRITICAL();
        printf("[MEMORY] WARNING: Double free in pool at %p\n", ptr);
        return RTOS_ERROR;
    }
    
    /* Mark block as free */
    pool->free_list[byte_idx] |= (1 << bit_idx);
    pool->free_blocks++;
    
    RTOS_EXIT_CRITICAL();
    
    printf("[MEMORY] Pool freed block %zu at %p\n", block_idx, ptr);
    
    return RTOS_OK;
}

/* Destroy memory pool */
void memory_pool_destroy(memory_pool_t *pool) {
    if (pool != NULL) {
        if (pool->pool_start != NULL) {
            rtos_free(pool->pool_start);
        }
        if (pool->free_list != NULL) {
            rtos_free(pool->free_list);
        }
        memset(pool, 0, sizeof(memory_pool_t));
        printf("[MEMORY] Pool destroyed\n");
    }
}

/* Calculate heap fragmentation */
static void calculate_fragmentation(memory_stats_t *stats) {
    memory_block_t *current = heap_head;
    size_t largest_free = 0;
    size_t total_free = 0;
    
    while (current != NULL) {
        if (current->is_free) {
            total_free += current->size;
            if (current->size > largest_free) {
                largest_free = current->size;
            }
        }
        current = current->next;
    }
    
    stats->largest_free_block = largest_free;
    
    /* Fragmentation = (1 - largest_free / total_free) * 100 */
    if (total_free > 0) {
        stats->fragmentation_percent = 
            (uint32_t)((1.0 - ((double)largest_free / total_free)) * 100.0);
    } else {
        stats->fragmentation_percent = 0;
    }
}

/* Get memory statistics */
void memory_get_stats(memory_stats_t *stats) {
    if (stats == NULL || !memory_initialized) {
        return;
    }
    
    RTOS_ENTER_CRITICAL();
    
    *stats = global_stats;
    calculate_fragmentation(stats);
    
    RTOS_EXIT_CRITICAL();
}

/* Print memory statistics */
void memory_print_stats(void) {
    memory_stats_t stats;
    memory_get_stats(&stats);
    
    printf("\n[MEMORY] ===== Memory Statistics =====\n");
    printf("  Total Heap Size:      %zu bytes\n", stats.total_heap_size);
    printf("  Used Heap:            %zu bytes\n", stats.used_heap_size);
    printf("  Free Heap:            %zu bytes\n", stats.free_heap_size);
    printf("  Largest Free Block:   %zu bytes\n", stats.largest_free_block);
    printf("  Total Allocations:    %u\n", stats.num_allocations);
    printf("  Total Frees:          %u\n", stats.num_frees);
    printf("  Fragmentation:        %u%%\n", stats.fragmentation_percent);
    printf("[MEMORY] ===============================\n\n");
}

/* Check for stack overflow */
bool memory_check_stack_overflow(uint32_t task_id) {
    tcb_t *tcb = task_get_tcb(task_id);
    if (tcb == NULL) {
        return false;
    }
    
    /* 
     * In a real embedded system, we would:
     * 1. Place a canary value at the bottom of each task's stack
     * 2. Periodically check if the canary is still intact
     * 3. Trigger an error handler if stack overflow detected
     * 
     * Stack grows downward, so overflow happens at lower addresses
     */
    
    uint32_t *stack_bottom = &tcb->stack[0];
    uint32_t canary = STACK_CANARY;
    
    /* Check first few words of stack */
    for (int i = 0; i < 4; i++) {
        if (stack_bottom[i] != canary && stack_bottom[i] != 0) {
            printf("[MEMORY] ERROR: Stack overflow detected in task '%s'!\n", 
                   tcb->name);
            return true;
        }
    }
    
    return false;
}