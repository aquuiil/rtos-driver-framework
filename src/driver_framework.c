/**
 * @file driver_framework.c
 * @brief Device Driver Framework - Implementation
 * 
 * Implements device registry and binding mechanism.
 * ZephyrOS parallel: This mirrors ZephyrOS device tree and driver binding.
 */

#include "driver_framework.h"
#include <stdio.h>
#include <string.h>

/* ==================== CONFIGURATION ==================== */

#define MAX_DRIVERS         16      /* Maximum number of registered drivers */
#define MAX_DEVICES         32      /* Maximum number of registered devices */

/* ==================== STATIC DATA ==================== */

static driver_t driver_registry[MAX_DRIVERS];
static uint32_t driver_count = 0;

static device_t device_registry[MAX_DEVICES];
static uint32_t device_count = 0;

/* ==================== DRIVER REGISTRY ==================== */

device_status_t driver_register(const driver_t *driver) {
    if (driver == NULL || driver->name == NULL) {
        return DEVICE_STATUS_INVALID_PARAM;
    }
    
    /* Check if driver already registered */
    for (uint32_t i = 0; i < driver_count; i++) {
        if (strcmp(driver_registry[i].name, driver->name) == 0) {
            printf("[Driver Framework] WARNING: Driver '%s' already registered\n", driver->name);
            return DEVICE_STATUS_ALREADY_INITIALIZED;
        }
    }
    
    if (driver_count >= MAX_DRIVERS) {
        printf("[Driver Framework] ERROR: Driver registry full (max %d)\n", MAX_DRIVERS);
        return DEVICE_STATUS_ERROR;
    }
    
    driver_registry[driver_count] = *driver;
    driver_count++;
    
    printf("[Driver Framework] Registered driver: %s\n", driver->name);
    return DEVICE_STATUS_OK;
}

static driver_t *driver_lookup(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < driver_count; i++) {
        if (strcmp(driver_registry[i].name, name) == 0) {
            return &driver_registry[i];
        }
    }
    
    return NULL;
}

/* ==================== DEVICE REGISTRY ==================== */

device_status_t device_register(device_t *dev) {
    if (dev == NULL || dev->name == NULL || dev->driver_name == NULL) {
        return DEVICE_STATUS_INVALID_PARAM;
    }
    
    /* Check if device already registered */
    for (uint32_t i = 0; i < device_count; i++) {
        if (strcmp(device_registry[i].name, dev->name) == 0) {
            printf("[Driver Framework] WARNING: Device '%s' already registered\n", dev->name);
            return DEVICE_STATUS_ALREADY_INITIALIZED;
        }
    }
    
    if (device_count >= MAX_DEVICES) {
        printf("[Driver Framework] ERROR: Device registry full (max %d)\n", MAX_DEVICES);
        return DEVICE_STATUS_ERROR;
    }
    
    device_registry[device_count] = *dev;
    device_count++;
    
    printf("[Driver Framework] Registered device: %s (driver: %s)\n", dev->name, dev->driver_name);
    return DEVICE_STATUS_OK;
}

device_t *device_get_by_name(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < device_count; i++) {
        if (strcmp(device_registry[i].name, name) == 0) {
            return &device_registry[i];
        }
    }
    
    return NULL;
}

device_t *device_get_by_driver_and_id(const char *driver_name, uint8_t id) {
    if (driver_name == NULL) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < device_count; i++) {
        if (strcmp(device_registry[i].driver_name, driver_name) == 0 &&
            device_registry[i].id == id) {
            return &device_registry[i];
        }
    }
    
    return NULL;
}

uint32_t device_get_count(void) {
    return device_count;
}

void device_print_registry(void) {
    printf("\n=== Device Registry ===\n");
    printf("Total devices: %d\n", device_count);
    for (uint32_t i = 0; i < device_count; i++) {
        const char *status = device_registry[i].initialized ? "INIT" : "UNINIT";
        printf("  [%d] %-20s | Driver: %-20s | ID: %d | %s\n",
               i,
               device_registry[i].name,
               device_registry[i].driver_name,
               device_registry[i].id,
               status);
    }
    printf("======================\n\n");
}

/* ==================== DEVICE INITIALIZATION ==================== */

device_status_t device_init_all(void) {
    printf("[Driver Framework] Initializing all devices...\n");
    
    device_status_t status = DEVICE_STATUS_OK;
    
    /* Initialize devices in priority order (lower init_priority first) */
    for (uint32_t iter = 0; iter < device_count; iter++) {
        device_t *next_dev = NULL;
        uint8_t min_priority = 255;
        uint32_t min_index = 0;
        
        /* Find next uninitialized device with lowest priority */
        for (uint32_t i = 0; i < device_count; i++) {
            if (!device_registry[i].initialized &&
                device_registry[i].init_priority < min_priority) {
                min_priority = device_registry[i].init_priority;
                next_dev = &device_registry[i];
                min_index = i;
            }
        }
        
        if (next_dev == NULL) {
            break;  /* All devices initialized */
        }
        
        /* Look up driver */
        driver_t *driver = driver_lookup(next_dev->driver_name);
        if (driver == NULL) {
            printf("[Driver Framework] ERROR: Driver '%s' not found for device '%s'\n",
                   next_dev->driver_name, next_dev->name);
            status = DEVICE_STATUS_NOT_FOUND;
            continue;
        }
        
        /* Call driver init if present */
        if (driver->init != NULL) {
            printf("[Driver Framework] Initializing device: %s (driver: %s)\n",
                   next_dev->name, driver->name);
            device_status_t dev_status = driver->init(next_dev);
            if (dev_status != DEVICE_STATUS_OK) {
                printf("[Driver Framework] ERROR: Device '%s' init failed with status %d\n",
                       next_dev->name, dev_status);
                status = dev_status;
                continue;
            }
        }
        
        device_registry[min_index].initialized = true;
        printf("[Driver Framework] Device '%s' initialized successfully\n", next_dev->name);
    }
    
    return status;
}

/* ==================== FRAMEWORK INITIALIZATION ==================== */

void device_framework_init(void) {
    printf("[Driver Framework] Initializing framework...\n");
    memset(driver_registry, 0, sizeof(driver_registry));
    memset(device_registry, 0, sizeof(device_registry));
    driver_count = 0;
    device_count = 0;
}

/* ==================== BUILT-IN DRIVER REGISTRATION ==================== */

/**
 * Forward declarations for built-in drivers
 * These will be implemented when drivers are refactored to use device framework
 */
extern device_status_t driver_uart_init(device_t *dev);
extern device_status_t driver_i2c_init(device_t *dev);
extern device_status_t driver_spi_init(device_t *dev);

void device_register_builtin_drivers(void) {
    static const driver_t uart_driver = {
        .name = "uart_driver",
        .init = driver_uart_init,
        .deinit = NULL,
    };
    
    static const driver_t i2c_driver = {
        .name = "i2c_driver",
        .init = driver_i2c_init,
        .deinit = NULL,
    };
    
    static const driver_t spi_driver = {
        .name = "spi_driver",
        .init = driver_spi_init,
        .deinit = NULL,
    };
    
    driver_register(&uart_driver);
    driver_register(&i2c_driver);
    driver_register(&spi_driver);
    
    printf("[Driver Framework] Built-in drivers registered\n");
}
