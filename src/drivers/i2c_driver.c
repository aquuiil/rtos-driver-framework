#include "drivers/i2c_driver.h"
#include "kernel/memory.h"
#include "driver_framework.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * ============================================================================
 * I2C PROTOCOL OVERVIEW
 * ============================================================================
 * 
 * I2C Communication Pattern:
 * 
 * START condition → Address + R/W bit → ACK/NACK → Data bytes → STOP condition
 * 
 * Example Write Transaction:
 * ┌─────┬──────────┬─────┬──────────┬─────┬──────────┬──────┐
 * │START│ 7-bit    │ W=0 │   ACK    │DATA │   ACK    │ STOP │
 * │     │ Address  │     │          │     │          │      │
 * └─────┴──────────┴─────┴──────────┴─────┴──────────┴──────┘
 * 
 * Example Read Transaction:
 * ┌─────┬──────────┬─────┬──────────┬──────────┬─────┬──────┐
 * │START│ 7-bit    │ R=1 │   ACK    │   DATA   │ ACK │ STOP │
 * │     │ Address  │     │          │          │     │      │
 * └─────┴──────────┴─────┴──────────┴──────────┴─────┴──────┘
 * 
 * Repeated START (for register read):
 * ┌─────┬────┬───┬────┬─────┬────┬─────┬────┬───┬────┬──────┬──────┬──────┐
 * │START│ADDR│ W │ACK │ REG │ACK │START│ADDR│ R │ACK │ DATA │ NACK │ STOP │
 * └─────┴────┴───┴────┴─────┴────┴─────┴────┴───┴────┴──────┴──────┴──────┘
 *        └─── Write register address ───┘ └──── Read data from register ────┘
 * 
 * START Condition: SDA falls while SCL is high
 * STOP Condition:  SDA rises while SCL is high
 * ACK:  SDA pulled low by receiver
 * NACK: SDA remains high (no acknowledgment)
 */

/* I2C handle structure */
struct i2c_handle {
    uint8_t i2c_id;
    i2c_config_t config;
    bool initialized;
    volatile bool busy;
};

/* Maximum I2C instances */
#define MAX_I2C_INSTANCES 4
static i2c_handle_t i2c_instances[MAX_I2C_INSTANCES] = {0};

/*
 * ============================================================================
 * I2C INITIALIZATION
 * ============================================================================
 */
i2c_handle_t* i2c_init(uint8_t i2c_id, const i2c_config_t *config) {
    if (i2c_id >= MAX_I2C_INSTANCES || config == NULL) {
        printf("[I2C%d] ERROR: Invalid parameters\n", i2c_id);
        return NULL;
    }
    
    i2c_handle_t *i2c = &i2c_instances[i2c_id];
    
    if (i2c->initialized) {
        printf("[I2C%d] WARNING: Already initialized\n", i2c_id);
        return i2c;
    }
    
    /* Initialize handle */
    memset(i2c, 0, sizeof(i2c_handle_t));
    i2c->i2c_id = i2c_id;
    i2c->config = *config;
    i2c->initialized = true;
    i2c->busy = false;
    
    printf("[I2C%d] Initialized:\n", i2c_id);
    printf("  Speed: %d Hz (%s mode)\n", 
           config->speed,
           config->speed == I2C_SPEED_STANDARD ? "Standard" :
           config->speed == I2C_SPEED_FAST ? "Fast" : "Fast+");
    printf("  Address mode: %s\n", 
           config->addr_mode == I2C_ADDR_7BIT ? "7-bit" : "10-bit");
    printf("  DMA: %s\n", config->enable_dma ? "enabled" : "disabled");
    printf("  Timeout: %d ms\n", config->timeout_ms);
    
    /*
     * =======================================================================
     * REAL HARDWARE INITIALIZATION (ARM Cortex-M Example - STM32)
     * =======================================================================
     * 
     * 1. ENABLE I2C PERIPHERAL CLOCK
     *    
     *    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
     * 
     * 2. CONFIGURE GPIO PINS
     *    Example for STM32 (PB6=SCL, PB7=SDA):
     *    
     *    // Enable GPIO clock
     *    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
     *    
     *    // Set alternate function mode
     *    GPIOB->MODER |= GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1;
     *    
     *    // Set to open-drain (required for I2C)
     *    GPIOB->OTYPER |= GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7;
     *    
     *    // Set pull-up resistors
     *    GPIOB->PUPDR |= GPIO_PUPDR_PUPDR6_0 | GPIO_PUPDR_PUPDR7_0;
     *    
     *    // Set AF4 for I2C1
     *    GPIOB->AFR[0] |= (4 << (6*4)) | (4 << (7*4));
     * 
     * 3. RESET I2C PERIPHERAL
     *    
     *    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
     *    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
     * 
     * 4. CONFIGURE I2C TIMING (for desired speed)
     *    
     *    // For STM32F4, calculate timing based on PCLK
     *    uint32_t pclk = 42000000;  // 42 MHz APB1
     *    uint32_t freq = pclk / 1000000;  // MHz
     *    
     *    I2C1->CR2 = freq;  // Set peripheral clock frequency
     *    
     *    if (config->speed == I2C_SPEED_STANDARD) {
     *        // Standard mode (100 kHz)
     *        uint32_t ccr = pclk / (2 * 100000);
     *        I2C1->CCR = ccr;
     *        I2C1->TRISE = freq + 1;
     *    } else if (config->speed == I2C_SPEED_FAST) {
     *        // Fast mode (400 kHz)
     *        uint32_t ccr = pclk / (3 * 400000);
     *        I2C1->CCR = ccr | I2C_CCR_FS;  // Fast mode bit
     *        I2C1->TRISE = ((freq * 300) / 1000) + 1;
     *    }
     * 
     * 5. CONFIGURE ADDRESSING MODE
     *    
     *    if (config->addr_mode == I2C_ADDR_10BIT) {
     *        I2C1->OAR1 |= I2C_OAR1_ADDMODE;  // 10-bit mode
     *    }
     * 
     * 6. ENABLE I2C PERIPHERAL
     *    
     *    I2C1->CR1 = I2C_CR1_PE;  // Peripheral enable
     * 
     * 7. CONFIGURE DMA (if enabled)
     *    
     *    if (config->enable_dma) {
     *        // Enable DMA for TX (Stream 6, Channel 1)
     *        DMA1_Stream6->PAR = (uint32_t)&I2C1->DR;
     *        DMA1_Stream6->CR = (1 << 25) |  // Channel 1
     *                           DMA_SxCR_MINC | // Memory increment
     *                           DMA_SxCR_DIR_0; // Memory to peripheral
     *        
     *        // Enable DMA for RX (Stream 5, Channel 1)
     *        DMA1_Stream5->PAR = (uint32_t)&I2C1->DR;
     *        DMA1_Stream5->CR = (1 << 25) |  // Channel 1
     *                           DMA_SxCR_MINC;  // Memory increment
     *        
     *        // Enable I2C DMA requests
     *        I2C1->CR2 |= I2C_CR2_DMAEN;
     *    }
     * 
     * =======================================================================
     */
    
    return i2c;
}

/*
 * ============================================================================
 * I2C DEINITIALIZATION
 * ============================================================================
 */
rtos_status_t i2c_deinit(i2c_handle_t *i2c) {
    if (i2c == NULL || !i2c->initialized) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /*
     * REAL HARDWARE:
     * I2C1->CR1 &= ~I2C_CR1_PE;  // Disable peripheral
     * RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;  // Disable clock
     */
    
    i2c->initialized = false;
    printf("[I2C%d] Deinitialized\n", i2c->i2c_id);
    
    return RTOS_OK;
}

/*
 * ============================================================================
 * I2C MASTER WRITE
 * ============================================================================
 */
rtos_status_t i2c_master_write(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    const uint8_t *data,
    size_t len
) {
    if (i2c == NULL || !i2c->initialized || data == NULL || len == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (i2c->busy) {
        return RTOS_ERROR_BUSY;
    }
    
    i2c->busy = true;
    
    printf("[I2C%d] WRITE to 0x%02X (%zu bytes): ", 
           i2c->i2c_id, slave_addr, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    
    /*
     * =======================================================================
     * REAL I2C WRITE TRANSACTION
     * =======================================================================
     * 
     * 1. GENERATE START CONDITION
     *    
     *    I2C1->CR1 |= I2C_CR1_START;
     *    
     *    // Wait for START condition to be sent
     *    while (!(I2C1->SR1 & I2C_SR1_SB));
     * 
     * 2. SEND SLAVE ADDRESS + WRITE BIT
     *    
     *    I2C1->DR = (slave_addr << 1) | 0;  // Write bit = 0
     *    
     *    // Wait for address to be sent
     *    while (!(I2C1->SR1 & I2C_SR1_ADDR));
     *    
     *    // Clear ADDR flag by reading SR1 and SR2
     *    (void)I2C1->SR1;
     *    (void)I2C1->SR2;
     * 
     * 3. CHECK FOR ACK/NACK
     *    
     *    if (I2C1->SR1 & I2C_SR1_AF) {
     *        // NACK received - slave not responding
     *        I2C1->SR1 &= ~I2C_SR1_AF;  // Clear flag
     *        I2C1->CR1 |= I2C_CR1_STOP;  // Generate STOP
     *        i2c->busy = false;
     *        return RTOS_ERROR;
     *    }
     * 
     * 4. SEND DATA BYTES
     *    
     *    for (size_t i = 0; i < len; i++) {
     *        // Wait for TXE (transmit register empty)
     *        while (!(I2C1->SR1 & I2C_SR1_TXE));
     *        
     *        // Write data byte
     *        I2C1->DR = data[i];
     *        
     *        // Check for errors
     *        if (I2C1->SR1 & I2C_SR1_AF) {
     *            // NACK received
     *            I2C1->SR1 &= ~I2C_SR1_AF;
     *            I2C1->CR1 |= I2C_CR1_STOP;
     *            i2c->busy = false;
     *            return RTOS_ERROR;
     *        }
     *    }
     *    
     *    // Wait for BTF (byte transfer finished)
     *    while (!(I2C1->SR1 & I2C_SR1_BTF));
     * 
     * 5. GENERATE STOP CONDITION
     *    
     *    I2C1->CR1 |= I2C_CR1_STOP;
     * 
     * 6. WITH DMA (Alternative):
     *    
     *    // Configure DMA
     *    DMA1_Stream6->M0AR = (uint32_t)data;
     *    DMA1_Stream6->NDTR = len;
     *    DMA1_Stream6->CR |= DMA_SxCR_EN;
     *    
     *    // Enable I2C DMA
     *    I2C1->CR2 |= I2C_CR2_DMAEN;
     *    
     *    // Wait for DMA completion (via interrupt or polling)
     * 
     * =======================================================================
     */
    
    /* Simulate transaction time */
    #ifdef _WIN32
    Sleep(len + 2);  // START + Address + Data + STOP
    #endif
    
    i2c->busy = false;
    
    return RTOS_OK;
}

/*
 * ============================================================================
 * I2C MASTER READ
 * ============================================================================
 */
rtos_status_t i2c_master_read(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t *buffer,
    size_t len
) {
    if (i2c == NULL || !i2c->initialized || buffer == NULL || len == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (i2c->busy) {
        return RTOS_ERROR_BUSY;
    }
    
    i2c->busy = true;
    
    /* Simulate reading data */
    for (size_t i = 0; i < len; i++) {
        buffer[i] = 0xAA + i;  // Dummy data
    }
    
    printf("[I2C%d] READ from 0x%02X (%zu bytes): ", 
           i2c->i2c_id, slave_addr, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    /*
     * =======================================================================
     * REAL I2C READ TRANSACTION
     * =======================================================================
     * 
     * 1. GENERATE START CONDITION
     *    I2C1->CR1 |= I2C_CR1_START;
     *    while (!(I2C1->SR1 & I2C_SR1_SB));
     * 
     * 2. SEND SLAVE ADDRESS + READ BIT
     *    I2C1->DR = (slave_addr << 1) | 1;  // Read bit = 1
     *    while (!(I2C1->SR1 & I2C_SR1_ADDR));
     *    (void)I2C1->SR1;
     *    (void)I2C1->SR2;
     * 
     * 3. ENABLE ACK FOR MULTI-BYTE READ
     *    if (len > 1) {
     *        I2C1->CR1 |= I2C_CR1_ACK;
     *    }
     * 
     * 4. READ DATA BYTES
     *    for (size_t i = 0; i < len; i++) {
     *        if (i == len - 1) {
     *            // Last byte - send NACK
     *            I2C1->CR1 &= ~I2C_CR1_ACK;
     *        }
     *        
     *        // Wait for RXNE (receive buffer not empty)
     *        while (!(I2C1->SR1 & I2C_SR1_RXNE));
     *        
     *        // Read data
     *        buffer[i] = I2C1->DR;
     *    }
     * 
     * 5. GENERATE STOP CONDITION
     *    I2C1->CR1 |= I2C_CR1_STOP;
     * 
     * =======================================================================
     */
    
    #ifdef _WIN32
    Sleep(len + 2);
    #endif
    
    i2c->busy = false;
    
    return RTOS_OK;
}

/*
 * ============================================================================
 * I2C COMBINED WRITE-READ (Common for register access)
 * ============================================================================
 */
rtos_status_t i2c_master_write_read(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    const uint8_t *write_data,
    size_t write_len,
    uint8_t *read_buffer,
    size_t read_len
) {
    if (i2c == NULL || !i2c->initialized) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (i2c->busy) {
        return RTOS_ERROR_BUSY;
    }
    
    i2c->busy = true;
    
    printf("[I2C%d] WRITE-READ to 0x%02X:\n", i2c->i2c_id, slave_addr);
    printf("  Write (%zu bytes): ", write_len);
    for (size_t i = 0; i < write_len; i++) {
        printf("%02X ", write_data[i]);
    }
    printf("\n");
    
    /* Simulate read data */
    for (size_t i = 0; i < read_len; i++) {
        read_buffer[i] = 0xBB + i;
    }
    
    printf("  Read (%zu bytes): ", read_len);
    for (size_t i = 0; i < read_len; i++) {
        printf("%02X ", read_buffer[i]);
    }
    printf("\n");
    
    /*
     * =======================================================================
     * REAL I2C WRITE-READ WITH REPEATED START
     * =======================================================================
     * 
     * // Write phase
     * I2C1->CR1 |= I2C_CR1_START;
     * while (!(I2C1->SR1 & I2C_SR1_SB));
     * I2C1->DR = (slave_addr << 1) | 0;  // Write
     * while (!(I2C1->SR1 & I2C_SR1_ADDR));
     * (void)I2C1->SR1; (void)I2C1->SR2;
     * 
     * for (size_t i = 0; i < write_len; i++) {
     *     while (!(I2C1->SR1 & I2C_SR1_TXE));
     *     I2C1->DR = write_data[i];
     * }
     * while (!(I2C1->SR1 & I2C_SR1_BTF));
     * 
     * // Repeated START (no STOP between write and read)
     * I2C1->CR1 |= I2C_CR1_START;
     * while (!(I2C1->SR1 & I2C_SR1_SB));
     * I2C1->DR = (slave_addr << 1) | 1;  // Read
     * while (!(I2C1->SR1 & I2C_SR1_ADDR));
     * (void)I2C1->SR1; (void)I2C1->SR2;
     * 
     * // Read phase
     * for (size_t i = 0; i < read_len; i++) {
     *     if (i == read_len - 1) {
     *         I2C1->CR1 &= ~I2C_CR1_ACK;  // NACK last byte
     *     }
     *     while (!(I2C1->SR1 & I2C_SR1_RXNE));
     *     read_buffer[i] = I2C1->DR;
     * }
     * 
     * I2C1->CR1 |= I2C_CR1_STOP;
     * 
     * =======================================================================
     */
    
    #ifdef _WIN32
    Sleep(write_len + read_len + 3);
    #endif
    
    i2c->busy = false;
    
    return RTOS_OK;
}

/*
 * ============================================================================
 * CONVENIENCE FUNCTIONS FOR REGISTER ACCESS
 * ============================================================================
 */

/* Write single register (common sensor operation) */
rtos_status_t i2c_write_register(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t reg_addr,
    uint8_t value
) {
    uint8_t data[2] = {reg_addr, value};
    return i2c_master_write(i2c, slave_addr, data, 2);
}

/* Read single register */
rtos_status_t i2c_read_register(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t reg_addr,
    uint8_t *value
) {
    return i2c_master_write_read(i2c, slave_addr, &reg_addr, 1, value, 1);
}

/* Read multiple consecutive registers (burst read) */
rtos_status_t i2c_read_registers(
    i2c_handle_t *i2c,
    uint16_t slave_addr,
    uint8_t reg_addr,
    uint8_t *buffer,
    size_t len
) {
    return i2c_master_write_read(i2c, slave_addr, &reg_addr, 1, buffer, len);
}

/*
 * ============================================================================
 * I2C BUS SCANNER
 * ============================================================================
 * Scans all possible I2C addresses to find connected devices
 */
rtos_status_t i2c_scan_bus(i2c_handle_t *i2c, uint8_t *found_devices, size_t *count) {
    if (i2c == NULL || !i2c->initialized || found_devices == NULL || count == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    printf("[I2C%d] Scanning bus...\n", i2c->i2c_id);
    
    *count = 0;
    
    /* Scan all 7-bit addresses (0x03 to 0x77) */
    for (uint16_t addr = 0x03; addr <= 0x77; addr++) {
        /*
         * REAL HARDWARE SCAN:
         * 
         * I2C1->CR1 |= I2C_CR1_START;
         * while (!(I2C1->SR1 & I2C_SR1_SB));
         * I2C1->DR = (addr << 1) | 0;  // Write bit
         * 
         * // Wait for address sent or ACK failure
         * uint32_t timeout = 1000;
         * while (!(I2C1->SR1 & (I2C_SR1_ADDR | I2C_SR1_AF)) && --timeout);
         * 
         * if (I2C1->SR1 & I2C_SR1_ADDR) {
         *     // ACK received - device found
         *     (void)I2C1->SR1; (void)I2C1->SR2;
         *     found_devices[*count] = addr;
         *     (*count)++;
         *     printf("  Found device at 0x%02X\n", addr);
         * } else if (I2C1->SR1 & I2C_SR1_AF) {
         *     // NACK - no device at this address
         *     I2C1->SR1 &= ~I2C_SR1_AF;
         * }
         * 
         * I2C1->CR1 |= I2C_CR1_STOP;
         */
        
        /* Simulation: "find" a few dummy devices */
        if (addr == 0x50 || addr == 0x68 || addr == 0x77) {
            found_devices[*count] = addr;
            (*count)++;
            printf("  Found device at 0x%02X\n", addr);
        }
    }
    
    printf("[I2C%d] Scan complete. Found %zu devices\n", i2c->i2c_id, *count);
    
    return RTOS_OK;
}

/* ============================================================================
 * DEVICE FRAMEWORK INTEGRATION
 * ============================================================================
 * 
 * ZephyrOS parallel: Mirrors how ZephyrOS drivers implement device.init
 * callback to integrate with the device tree/device framework.
 */

/**
 * Driver initialization callback for device framework
 * 
 * Called by device_init_all() when an I2C device is being initialized.
 * 
 * @param dev Device structure with platform_data containing i2c_config_t*
 * @return DEVICE_STATUS_OK on success
 */
device_status_t driver_i2c_init(device_t *dev) {
    if (dev == NULL) {
        return DEVICE_STATUS_INVALID_PARAM;
    }
    
    printf("[I2C Driver] Initializing device: %s (ID: %d)\n", dev->name, dev->id);
    
    /* platform_data should contain a pointer to i2c_config_t */
    if (dev->platform_data == NULL) {
        printf("[I2C Driver] ERROR: No platform_data provided\n");
        return DEVICE_STATUS_INVALID_PARAM;
    }
    
    i2c_config_t *config = (i2c_config_t *)dev->platform_data;
    
    /* Initialize the I2C using existing i2c_init() */
    i2c_handle_t *i2c_handle = i2c_init(dev->id, config);
    
    if (i2c_handle == NULL) {
        printf("[I2C Driver] ERROR: i2c_init() failed\n");
        return DEVICE_STATUS_ERROR;
    }
    
    /* Store the handle in driver_data for later retrieval */
    dev->driver_data = (void *)i2c_handle;
    
    printf("[I2C Driver] Device %s initialized successfully\n", dev->name);
    return DEVICE_STATUS_OK;
}