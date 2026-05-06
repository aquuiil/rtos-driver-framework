/**
 * @file driver_framework.h
 * @brief Device Driver Framework - Abstraction layer for device management
 * 
 * This module implements a ZephyrOS-inspired device abstraction layer enabling:
 * - Device registration and discovery
 * - Driver registration and binding
 * - Automatic device initialization
 * - Platform-agnostic device enumeration
 * 
 * ZephyrOS parallel: This mirrors ZephyrOS device model where drivers
 * register themselves and devices are matched to drivers via device tree or
 * platform data. Here we use a simpler static device table.
 */

#ifndef DRIVER_FRAMEWORK_H
#define DRIVER_FRAMEWORK_H

#include "rtos_types.h"

/* ==================== TYPE DEFINITIONS ==================== */

/**
 * @brief Device status codes
 */
typedef enum {
    DEVICE_STATUS_OK = 0,
    DEVICE_STATUS_ERROR,
    DEVICE_STATUS_NOT_FOUND,
    DEVICE_STATUS_ALREADY_INITIALIZED,
    DEVICE_STATUS_INVALID_PARAM,
} device_status_t;

/**
 * @brief Device structure - represents a hardware device instance
 * 
 * ZephyrOS equivalent: struct device in ZephyrOS
 * 
 * Each device has:
 * - name: unique identifier for lookup (e.g., "uart_0", "i2c_0")
 * - id: numeric identifier for the device instance
 * - driver_name: name of driver managing this device (e.g., "uart_driver")
 * - platform_data: device-specific configuration (HAL address, pin config, etc.)
 * - driver_data: runtime state allocated by driver (opaque handle)
 */
typedef struct device {
    const char *name;           /**< Unique device name (e.g., "uart_0") */
    uint8_t id;                 /**< Device instance number */
    const char *driver_name;    /**< Driver name (e.g., "uart_driver") */
    void *platform_data;        /**< Hardware-specific config (driver interprets) */
    void *driver_data;          /**< Runtime driver state (opaque to framework) */
    uint8_t init_priority;      /**< Initialization order (lower = earlier) */
    bool initialized;           /**< Has device been initialized? */
} device_t;

/**
 * @brief Driver initialization callback
 * 
 * Called when device is initialized. Driver should:
 * 1. Read device->platform_data for hardware configuration
 * 2. Set up hardware via HAL
 * 3. Store runtime state in device->driver_data (e.g., handle)
 * 4. Return status
 * 
 * ZephyrOS equivalent: struct device_driver.init callback
 */
typedef device_status_t (*driver_init_t)(device_t *dev);

/**
 * @brief Driver deinitialization callback
 * 
 * ZephyrOS equivalent: struct device_driver.deinit callback
 */
typedef device_status_t (*driver_deinit_t)(device_t *dev);

/**
 * @brief Driver structure - describes a device driver
 * 
 * ZephyrOS equivalent: struct device_driver in ZephyrOS
 */
typedef struct {
    const char *name;           /**< Driver name (e.g., "uart_driver") */
    driver_init_t init;         /**< Initialization callback */
    driver_deinit_t deinit;     /**< Deinitialization callback */
} driver_t;

/* ==================== API FUNCTIONS ==================== */

/**
 * @brief Initialize the device framework
 * 
 * Must be called before any other device framework functions.
 */
void device_framework_init(void);

/**
 * @brief Register built-in drivers (UART, I2C, SPI)
 * 
 * Called after device_framework_init() to register drivers for standard peripherals.
 */
void device_register_builtin_drivers(void);

/**
 * @brief Register a driver
 * 
 * @param driver Pointer to driver_t structure with name and callbacks
 * @return DEVICE_STATUS_OK on success
 * 
 * ZephyrOS parallel: Building blocks for device tree binding
 */
device_status_t driver_register(const driver_t *driver);

/**
 * @brief Register a device in the platform device table
 * 
 * @param dev Device to register
 * @return DEVICE_STATUS_OK on success, DEVICE_STATUS_ALREADY_INITIALIZED if exists
 */
device_status_t device_register(device_t *dev);

/**
 * @brief Initialize all registered devices in priority order
 * 
 * Scans device table, matches drivers, and calls driver->init() for each device.
 * Devices are initialized in order of init_priority (lower = earlier).
 * 
 * ZephyrOS parallel: init_result_t post_kernel in ZephyrOS device tree
 */
device_status_t device_init_all(void);

/**
 * @brief Get device by name
 * 
 * @param name Device name (e.g., "uart_0")
 * @return Pointer to device_t on success, NULL if not found
 * 
 * ZephyrOS parallel: device_get_binding() in ZephyrOS
 */
device_t *device_get_by_name(const char *name);

/**
 * @brief Get device by driver name and ID
 * 
 * @param driver_name Driver name (e.g., "uart_driver")
 * @param id Device instance ID
 * @return Pointer to device_t on success, NULL if not found
 */
device_t *device_get_by_driver_and_id(const char *driver_name, uint8_t id);

/**
 * @brief Get number of registered devices
 */
uint32_t device_get_count(void);

/**
 * @brief Print device registry (debug function)
 * 
 * Enumerates and prints all registered devices and their status.
 */
void device_print_registry(void);

#endif /* DRIVER_FRAMEWORK_H */
