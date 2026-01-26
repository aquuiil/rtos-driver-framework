#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "rtos_types.h"

/* I2C speed modes */
typedef enum {
    I2C_SPEED_STANDARD = 100000,    // 100 kHz
    I2C_SPEED_FAST = 400000,        // 400 kHz
    I2C_SPEED_FAST_PLUS = 1000000   // 1 MHz
} i2c_speed_t;

/* I2C addressing mode */
typedef enum {
    I2C_ADDR_7BIT = 0,
    I2C_ADDR_10BIT = 1
} i2c_addr_mode_t;

/* I2C configuration */
typedef struct {
    i2c_speed_t speed;              // Bus speed
    i2c_addr_mode_t addr_mode;      // 7-bit or 10-bit addressing
    bool enable_dma;                // Enable DMA transfers
    uint16_t timeout_ms;            // Transaction timeout
} i2c_config_t;

/* I2C handle */
typedef struct i2c_handle i2c_handle_t;

/* I2C API */
i2c_handle_t* i2c_init(uint8_t i2c_id, const i2c_config_t *config);
rtos_status_t i2c_deinit(i2c_handle_t *i2c);

/* Master mode operations */
rtos_status_t i2c_master_write(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    const uint8_t *data,
    size_t len
);

rtos_status_t i2c_master_read(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t *buffer,
    size_t len
);

rtos_status_t i2c_master_write_read(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    const uint8_t *write_data,
    size_t write_len,
    uint8_t *read_buffer,
    size_t read_len
);

/* Register read/write helpers (common pattern for sensors) */
rtos_status_t i2c_write_register(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t reg_addr,
    uint8_t value
);

rtos_status_t i2c_read_register(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t reg_addr,
    uint8_t *value
);

rtos_status_t i2c_read_registers(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t reg_addr,
    uint8_t *buffer,
    size_t len
);

/* Bus control */
rtos_status_t i2c_scan_bus(i2c_handle_t *i2c, uint8_t *found_devices, size_t *count);

#endif /* I2C_DRIVER_H */