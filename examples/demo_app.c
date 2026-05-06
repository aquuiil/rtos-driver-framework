#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "kernel/memory.h"
#include "drivers/uart_driver.h"
#include "drivers/i2c_driver.h"
#include "drivers/spi_driver.h"
#include "ipc/message_queue.h"
#include "ipc/semaphore.h"
#include "ipc/mutex.h"
#include "hal/arm_cortex_m4.h"
#include "bootloader/bootloader.h"
#include "driver_framework.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Task functions */
void task_led_blink(void *params);
void task_uart_comm(void *params);
void task_sensor_read(void *params);

/* Global resources */
static uart_handle_t *uart0 = NULL;
static i2c_handle_t *i2c0 = NULL;
static semaphore_t *sensor_sem = NULL;
static mutex_t *uart_mutex = NULL;

/* LED blink task */
void task_led_blink(void *params) {
    (void)params;
    uint32_t count = 0;
    
    printf("[LED_TASK] Started\n");
    
    while (1) {
        printf("[LED_TASK] Blink %u\n", count++);
        task_delay(MS_TO_TICKS(500));  // 500ms
    }
}

/* UART communication task */
void task_uart_comm(void *params) {
    (void)params;
    
    printf("[UART_TASK] Started\n");
    
    while (1) {
        mutex_lock(uart_mutex, RTOS_WAIT_FOREVER);
        
        const char *msg = "Hello from RTOS!\n";
        uart_write(uart0, (const uint8_t*)msg, strlen(msg));
        
        mutex_unlock(uart_mutex);
        
        task_delay(MS_TO_TICKS(1000));  // 1 second
    }
}

/* Sensor reading task */
void task_sensor_read(void *params) {
    (void)params;
    uint8_t sensor_data[6];
    
    printf("[SENSOR_TASK] Started\n");
    
    while (1) {
        sem_take(sensor_sem, RTOS_WAIT_FOREVER);
        
        // Read from I2C sensor (simulated)
        i2c_read_registers(i2c0, 0x68, 0x3B, sensor_data, 6);
        
        printf("[SENSOR_TASK] Sensor data: ");
        for (int i = 0; i < 6; i++) {
            printf("%02X ", sensor_data[i]);
        }
        printf("\n");
        
        sem_give(sensor_sem);
        
        task_delay(MS_TO_TICKS(100));  // 100ms
    }
}

int main(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║  Custom RTOS with Device Driver Framework            ║\n");
    printf("║  Embedded Systems Project                             ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    /* Initialize bootloader */
    bootloader_init();
    
    firmware_header_t fw_header = {
        .magic = 0xDEADC0DE,
        .version = 1,
        .size = 32768,
        .crc32 = 0x12345678,
        .entry_point = 0x08008000
    };
    
    if (bootloader_verify_image(&fw_header) != RTOS_OK) {
        printf("Bootloader verification failed!\n");
        return -1;
    }
    
    /* Initialize HAL */
    hal_init();
    
    /* Initialize memory management */
    memory_init();
    
    /* Initialize task system */
    task_init();
    
    /* Initialize scheduler */
    scheduler_init();
    
    /* ============== DEVICE FRAMEWORK SETUP ============== */
    printf("[MAIN] Initializing Device Framework...\n");
    
    /* Initialize device framework */
    device_framework_init();
    
    /* Register built-in drivers */
    device_register_builtin_drivers();
    
    /* Create platform device configurations */
    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD_115200,
        .parity = UART_PARITY_NONE,
        .stop_bits = UART_STOPBITS_1,
        .data_bits = UART_DATABITS_8,
        .enable_dma = false,
        .enable_interrupts = true,
        .rx_buffer_size = 256,
        .tx_buffer_size = 256
    };
    
    i2c_config_t i2c_cfg = {
        .speed = I2C_SPEED_FAST,
        .addr_mode = I2C_ADDR_7BIT,
        .enable_dma = false,
        .timeout_ms = 100
    };
    
    spi_config_t spi_cfg = {
        .mode = SPI_MODE_0,
        .speed = SPI_SPEED_HIGH,
        .enable_dma = false
    };
    
    /* Register platform devices (these tell drivers how to initialize) */
    device_t uart_dev = {
        .name = "uart_0",
        .id = 0,
        .driver_name = "uart_driver",
        .platform_data = &uart_cfg,
        .init_priority = 10,
        .initialized = false
    };
    device_register(&uart_dev);
    
    device_t i2c_dev = {
        .name = "i2c_0",
        .id = 0,
        .driver_name = "i2c_driver",
        .platform_data = &i2c_cfg,
        .init_priority = 20,
        .initialized = false
    };
    device_register(&i2c_dev);
    
    device_t spi_dev = {
        .name = "spi_0",
        .id = 0,
        .driver_name = "spi_driver",
        .platform_data = &spi_cfg,
        .init_priority = 30,
        .initialized = false
    };
    device_register(&spi_dev);
    
    /* Initialize all devices (drivers will be called automatically) */
    device_init_all();
    
    /* Get device handles for use in tasks */
    device_t *uart_device = device_get_by_name("uart_0");
    device_t *i2c_device = device_get_by_name("i2c_0");
    device_t *spi_device = device_get_by_name("spi_0");
    
    /* Extract handles from driver_data */
    uart_handle_t *uart0 = (uart_handle_t *)uart_device->driver_data;
    i2c_handle_t *i2c0 = (i2c_handle_t *)i2c_device->driver_data;
    spi_handle_t *spi0 = (spi_handle_t *)spi_device->driver_data;
    
    /* Print device registry for verification */
    device_print_registry();
    
    /* ============== IPC SETUP ============== */
    printf("[MAIN] Creating IPC objects (semaphore, mutex)...\n");
    
    /* Create IPC objects */
    sensor_sem = sem_create(1, 1);
    uart_mutex = mutex_create();
    
    /* Create tasks */
    uint32_t task_id;
    
    task_create(task_led_blink, "LED_Blink", TASK_PRIORITY_LOW, NULL, &task_id);
    task_create(task_uart_comm, "UART_Comm", TASK_PRIORITY_NORMAL, NULL, &task_id);
    task_create(task_sensor_read, "Sensor_Read", TASK_PRIORITY_HIGH, NULL, &task_id);
    
    /* Print memory stats */
    memory_print_stats();
    
    /* Initialize SysTick for 1ms */
    hal_systick_init(1000);
    
    /* Start scheduler */
    printf("\n[MAIN] Starting scheduler...\n\n");
    scheduler_start();
    
    /* Simulate running for a while */
    #ifdef _WIN32
    Sleep(10000);  // Run for 10 seconds
    #endif
    
    /* Print final stats */
    printf("\n[MAIN] Stopping demo...\n");
    memory_print_stats();
    
    /* Cleanup */
    spi_deinit(spi0);
    i2c_deinit(i2c0);
    uart_deinit(uart0);
    sem_destroy(sensor_sem);
    mutex_destroy(uart_mutex);
    
    printf("\n[MAIN] Demo complete!\n");
    
    return 0;
}