#include "kernel/memory.h"
#include "kernel/task.h"
#include "kernel/scheduler.h"
#include "drivers/uart_driver.h"
#include "ipc/semaphore.h"
#include <stdio.h>
#include <assert.h>

void test_memory(void) {
    printf("\n=== Testing Memory Management ===\n");
    
    memory_init();
    
    void *ptr1 = rtos_malloc(100);
    assert(ptr1 != NULL);
    printf("✓ Allocated 100 bytes\n");
    
    void *ptr2 = rtos_malloc(200);
    assert(ptr2 != NULL);
    printf("✓ Allocated 200 bytes\n");
    
    rtos_free(ptr1);
    printf("✓ Freed first allocation\n");
    
    void *ptr3 = rtos_malloc(50);
    assert(ptr3 != NULL);
    printf("✓ Allocated 50 bytes (reused space)\n");
    
    rtos_free(ptr2);
    rtos_free(ptr3);
    
    memory_print_stats();
    printf("✓ Memory tests passed\n");
}

void test_uart(void) {
    printf("\n=== Testing UART Driver ===\n");
    
    uart_config_t cfg = {
        .baud_rate = UART_BAUD_115200,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOPBITS_1,
        .data_bits = UART_DATABITS_8,
        .enable_dma = false,
        .enable_interrupts = true,
        .rx_buffer_size = 128,
        .tx_buffer_size = 128
    };
    
    uart_handle_t *uart = uart_init(0, &cfg);
    assert(uart != NULL);
    printf("✓ UART initialized\n");
    
    const uint8_t test_data[] = "Test";
    rtos_status_t status = uart_write(uart, test_data, 4);
    assert(status == RTOS_OK);
    printf("✓ UART write successful\n");
    
    uart_deinit(uart);
    printf("✓ UART tests passed\n");
}

void test_semaphore(void) {
    printf("\n=== Testing Semaphores ===\n");
    
    memory_init();
    
    semaphore_t *sem = sem_create(1, 1);
    assert(sem != NULL);
    printf("✓ Semaphore created\n");
    
    rtos_status_t status = sem_take(sem, 0);
    assert(status == RTOS_OK);
    printf("✓ Semaphore taken\n");
    
    status = sem_give(sem);
    assert(status == RTOS_OK);
    printf("✓ Semaphore given\n");
    
    sem_destroy(sem);
    printf("✓ Semaphore tests passed\n");
}

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║   RTOS Test Suite                     ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    
    test_memory();
    test_uart();
    test_semaphore();

    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║   ALL TESTS PASSED ✓                  ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("\n");

    return 0;
}