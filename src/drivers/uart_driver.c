#include "drivers/uart_driver.h"
#include "kernel/memory.h"
#include "driver_framework.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * ============================================================================
 * CIRCULAR BUFFER IMPLEMENTATION
 * ============================================================================
 * 
 * Circular buffers (ring buffers) are essential in embedded systems for:
 * - Efficient FIFO (First-In-First-Out) data storage
 * - No need to shift data when elements are removed
 * - Perfect for interrupt-driven I/O (producer-consumer pattern)
 * 
 * Structure:
 *   - Fixed-size array
 *   - Head pointer (where to write next)
 *   - Tail pointer (where to read next)
 *   - Count (number of elements currently stored)
 * 
 * When buffer is full: head catches up to tail
 * When buffer is empty: head == tail && count == 0
 */

/* Circular buffer structure */
typedef struct {
    uint8_t *buffer;        // Pointer to buffer memory
    size_t size;            // Total buffer size
    volatile size_t head;   // Write position (producer)
    volatile size_t tail;   // Read position (consumer)
    volatile size_t count;  // Current number of elements
} circular_buffer_t;

/*
 * ============================================================================
 * UART HANDLE STRUCTURE
 * ============================================================================
 * 
 * This structure represents a single UART instance and contains:
 * - Configuration parameters
 * - RX/TX circular buffers
 * - Callback functions for events
 * - State flags
 */

struct uart_handle {
    uint8_t uart_id;                // UART instance number (0-3)
    uart_config_t config;           // Configuration (baud, parity, etc.)
    circular_buffer_t rx_buffer;    // Receive circular buffer
    circular_buffer_t tx_buffer;    // Transmit circular buffer
    uart_callback_t rx_callback;    // RX complete callback
    void *rx_callback_data;         // User data for RX callback
    uart_callback_t tx_callback;    // TX complete callback
    void *tx_callback_data;         // User data for TX callback
    bool initialized;               // Initialization flag
    volatile bool tx_busy;          // TX in progress flag (for DMA)
};

/* Maximum number of UART instances */
#define MAX_UART_INSTANCES 4
static uart_handle_t uart_instances[MAX_UART_INSTANCES] = {0};

/*
 * ============================================================================
 * CIRCULAR BUFFER OPERATIONS
 * ============================================================================
 */

/* Initialize circular buffer */
static void circ_buf_init(circular_buffer_t *cb, size_t size) {
    cb->buffer = (uint8_t*)rtos_malloc(size);
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

/* Deinitialize circular buffer */
static void circ_buf_deinit(circular_buffer_t *cb) {
    if (cb->buffer) {
        rtos_free(cb->buffer);
        cb->buffer = NULL;
    }
}

/* Put one byte into circular buffer */
static bool circ_buf_put(circular_buffer_t *cb, uint8_t data) {
    RTOS_ENTER_CRITICAL();  // Disable interrupts for thread safety
    
    if (cb->count >= cb->size) {
        RTOS_EXIT_CRITICAL();
        return false;  // Buffer full
    }
    
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->size;  // Wrap around
    cb->count++;
    
    RTOS_EXIT_CRITICAL();
    return true;
}

/* Get one byte from circular buffer */
static bool circ_buf_get(circular_buffer_t *cb, uint8_t *data) {
    RTOS_ENTER_CRITICAL();
    
    if (cb->count == 0) {
        RTOS_EXIT_CRITICAL();
        return false;  // Buffer empty
    }
    
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;  // Wrap around
    cb->count--;
    
    RTOS_EXIT_CRITICAL();
    return true;
}

/* Get number of bytes available in buffer */
static size_t circ_buf_available(circular_buffer_t *cb) {
    return cb->count;
}

/* Flush (clear) circular buffer */
static void circ_buf_flush(circular_buffer_t *cb) {
    RTOS_ENTER_CRITICAL();
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    RTOS_EXIT_CRITICAL();
}

/*
 * ============================================================================
 * UART DRIVER API IMPLEMENTATION
 * ============================================================================
 */

/* 
 * Initialize UART peripheral
 * ---------------------------
 * Configures UART hardware with specified parameters and allocates buffers.
 * 
 * Parameters:
 *   uart_id: UART instance number (0-3)
 *   config:  Configuration structure (baud rate, parity, etc.)
 * 
 * Returns:
 *   Pointer to UART handle on success, NULL on failure
 */
uart_handle_t* uart_init(uint8_t uart_id, const uart_config_t *config) {
    if (uart_id >= MAX_UART_INSTANCES || config == NULL) {
        printf("[UART%d] ERROR: Invalid parameters\n", uart_id);
        return NULL;
    }
    
    uart_handle_t *uart = &uart_instances[uart_id];
    
    if (uart->initialized) {
        printf("[UART%d] WARNING: Already initialized\n", uart_id);
        return uart;
    }
    
    /* Initialize handle */
    memset(uart, 0, sizeof(uart_handle_t));
    uart->uart_id = uart_id;
    uart->config = *config;
    
    /* Initialize circular buffers */
    circ_buf_init(&uart->rx_buffer, config->rx_buffer_size);
    circ_buf_init(&uart->tx_buffer, config->tx_buffer_size);
    
    if (uart->rx_buffer.buffer == NULL || uart->tx_buffer.buffer == NULL) {
        printf("[UART%d] ERROR: Failed to allocate buffers\n", uart_id);
        circ_buf_deinit(&uart->rx_buffer);
        circ_buf_deinit(&uart->tx_buffer);
        return NULL;
    }
    
    uart->initialized = true;
    uart->tx_busy = false;
    
    printf("[UART%d] Initialized:\n", uart_id);
    printf("  Baud: %d, Parity: %d, Stop: %d, Data: %d\n",
           config->baud_rate, config->parity, config->stop_bits, config->data_bits);
    printf("  DMA: %s, Interrupts: %s\n",
           config->enable_dma ? "enabled" : "disabled",
           config->enable_interrupts ? "enabled" : "disabled");
    printf("  RX Buffer: %d bytes, TX Buffer: %d bytes\n",
           config->rx_buffer_size, config->tx_buffer_size);
    
    /*
     * =======================================================================
     * REAL HARDWARE INITIALIZATION (ARM Cortex-M Example)
     * =======================================================================
     * 
     * In a real embedded system, here we would configure hardware registers:
     * 
     * 1. ENABLE UART PERIPHERAL CLOCK
     *    Example for STM32 (USART2 on APB1 bus):
     *    
     *    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
     * 
     * 2. CONFIGURE GPIO PINS FOR UART
     *    Example for STM32 (PA2=TX, PA3=RX):
     *    
     *    // Enable GPIO clock
     *    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
     *    
     *    // Set alternate function mode
     *    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1);
     *    
     *    // Set AF7 for USART2
     *    GPIOA->AFR[0] |= (7 << (2*4)) | (7 << (3*4));
     *    
     *    // Set speed to high
     *    GPIOA->OSPEEDR |= (GPIO_OSPEEDER_OSPEEDR2 | GPIO_OSPEEDER_OSPEEDR3);
     * 
     * 3. CONFIGURE BAUD RATE
     *    Baud Rate = fclk / (16 * USARTDIV)
     *    
     *    uint32_t apb1_freq = 42000000;  // 42 MHz
     *    uint32_t usartdiv = apb1_freq / config->baud_rate;
     *    USART2->BRR = usartdiv;
     * 
     * 4. CONFIGURE FRAME FORMAT
     *    
     *    uint32_t cr1 = 0;
     *    
     *    // Data bits
     *    if (config->data_bits == UART_DATABITS_9) {
     *        cr1 |= USART_CR1_M;  // 9-bit mode
     *    }
     *    
     *    // Parity
     *    if (config->parity != UART_PARITY_NONE) {
     *        cr1 |= USART_CR1_PCE;  // Enable parity
     *        if (config->parity == UART_PARITY_ODD) {
     *            cr1 |= USART_CR1_PS;  // Odd parity
     *        }
     *    }
     *    
     *    USART2->CR1 = cr1;
     * 
     * 5. CONFIGURE STOP BITS
     *    
     *    if (config->stop_bits == UART_STOPBITS_2) {
     *        USART2->CR2 |= USART_CR2_STOP_1;  // 2 stop bits
     *    }
     * 
     * 6. ENABLE INTERRUPTS (if configured)
     *    
     *    if (config->enable_interrupts) {
     *        // Enable RXNE interrupt (RX not empty)
     *        USART2->CR1 |= USART_CR1_RXNEIE;
     *        
     *        // Enable interrupt in NVIC
     *        NVIC_SetPriority(USART2_IRQn, 5);
     *        NVIC_EnableIRQ(USART2_IRQn);
     *    }
     * 
     * 7. CONFIGURE DMA (if enabled)
     *    
     *    if (config->enable_dma) {
     *        // Enable DMA clock
     *        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
     *        
     *        // Configure DMA for UART TX (DMA1, Stream 6, Channel 4)
     *        DMA1_Stream6->PAR = (uint32_t)&USART2->DR;  // Peripheral address
     *        DMA1_Stream6->CR = (4 << 25) |  // Channel 4
     *                           DMA_SxCR_MINC |  // Memory increment
     *                           DMA_SxCR_DIR_0 | // Memory to peripheral
     *                           DMA_SxCR_TCIE;   // Transfer complete interrupt
     *        
     *        // Enable USART DMA transmitter
     *        USART2->CR3 |= USART_CR3_DMAT;
     *        
     *        // Enable DMA interrupt in NVIC
     *        NVIC_SetPriority(DMA1_Stream6_IRQn, 5);
     *        NVIC_EnableIRQ(DMA1_Stream6_IRQn);
     *    }
     * 
     * 8. ENABLE UART PERIPHERAL
     *    
     *    USART2->CR1 |= USART_CR1_UE;   // Enable USART
     *    USART2->CR1 |= USART_CR1_TE;   // Enable transmitter
     *    USART2->CR1 |= USART_CR1_RE;   // Enable receiver
     * 
     * =======================================================================
     */
    
    return uart;
}

/* 
 * Deinitialize UART peripheral
 * -----------------------------
 * Disables UART and frees allocated resources.
 */
rtos_status_t uart_deinit(uart_handle_t *uart) {
    if (uart == NULL || !uart->initialized) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    /*
     * REAL HARDWARE DEINITIALIZATION:
     * 
     * // Disable UART
     * USART2->CR1 &= ~USART_CR1_UE;
     * 
     * // Disable interrupts
     * NVIC_DisableIRQ(USART2_IRQn);
     * 
     * // Disable DMA
     * if (uart->config.enable_dma) {
     *     DMA1_Stream6->CR &= ~DMA_SxCR_EN;
     *     NVIC_DisableIRQ(DMA1_Stream6_IRQn);
     * }
     * 
     * // Disable peripheral clock (optional, saves power)
     * RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;
     */
    
    /* Free buffers */
    circ_buf_deinit(&uart->rx_buffer);
    circ_buf_deinit(&uart->tx_buffer);
    
    uart->initialized = false;
    
    printf("[UART%d] Deinitialized\n", uart->uart_id);
    
    return RTOS_OK;
}

/* 
 * Write data to UART (blocking)
 * ------------------------------
 * Writes data to TX buffer and transmits via UART.
 * 
 * Parameters:
 *   uart: UART handle
 *   data: Pointer to data to transmit
 *   len:  Number of bytes to transmit
 * 
 * Returns:
 *   RTOS_OK on success, error code on failure
 */
rtos_status_t uart_write(uart_handle_t *uart, const uint8_t *data, size_t len) {
    if (uart == NULL || !uart->initialized || data == NULL || len == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    printf("[UART%d] TX (%zu bytes): ", uart->uart_id, len);
    
    for (size_t i = 0; i < len; i++) {
        /* Put data in TX buffer */
        if (!circ_buf_put(&uart->tx_buffer, data[i])) {
            printf("\n[UART%d] ERROR: TX buffer full\n", uart->uart_id);
            return RTOS_ERROR_BUSY;
        }
        
        /* Simulate transmission - print hex value */
        printf("%02X ", data[i]);
        
        /*
         * REAL HARDWARE TRANSMISSION:
         * 
         * if (config->enable_interrupts) {
         *     // Interrupt-driven mode
         *     // Enable TXE interrupt - ISR will send data from buffer
         *     USART2->CR1 |= USART_CR1_TXEIE;
         * } else {
         *     // Polling mode
         *     // Wait until TX register is empty
         *     while (!(USART2->SR & USART_SR_TXE));
         *     
         *     // Write data to data register
         *     USART2->DR = data[i];
         *     
         *     // Wait until transmission complete
         *     while (!(USART2->SR & USART_SR_TC));
         * }
         */
    }
    
    printf("(ASCII: \"");
    for (size_t i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            printf("%c", data[i]);
        } else {
            printf(".");
        }
    }
    printf("\")\n");
    
    /* Trigger TX complete callback if set */
    if (uart->tx_callback) {
        uart->tx_callback(uart, uart->tx_callback_data);
    }
    
    return RTOS_OK;
}

/* 
 * Read data from UART
 * -------------------
 * Reads available data from RX buffer.
 * 
 * Parameters:
 *   uart:       UART handle
 *   buffer:     Buffer to store received data
 *   len:        Maximum number of bytes to read
 *   bytes_read: Pointer to store actual number of bytes read
 * 
 * Returns:
 *   RTOS_OK on success, error code on failure
 */
rtos_status_t uart_read(uart_handle_t *uart, uint8_t *buffer, size_t len, size_t *bytes_read) {
    if (uart == NULL || !uart->initialized || buffer == NULL || bytes_read == NULL) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    *bytes_read = 0;
    
    /* Read available data from RX buffer */
    for (size_t i = 0; i < len; i++) {
        if (!circ_buf_get(&uart->rx_buffer, &buffer[i])) {
            break;  // No more data available
        }
        (*bytes_read)++;
    }
    
    if (*bytes_read > 0) {
        printf("[UART%d] RX: Read %zu bytes\n", uart->uart_id, *bytes_read);
    }
    
    /*
     * REAL HARDWARE READING:
     * 
     * In interrupt-driven mode, the ISR already put data in rx_buffer.
     * In polling mode:
     * 
     * for (size_t i = 0; i < len; i++) {
     *     // Wait for data to be received
     *     while (!(USART2->SR & USART_SR_RXNE));
     *     
     *     // Read data from data register
     *     buffer[i] = USART2->DR;
     *     (*bytes_read)++;
     * }
     */
    
    return RTOS_OK;
}

/* 
 * Write data using DMA
 * --------------------
 * Initiates DMA transfer for high-speed transmission.
 * 
 * DMA (Direct Memory Access) allows data transfer between memory and
 * peripherals without CPU intervention, freeing CPU for other tasks.
 */
rtos_status_t uart_write_dma(uart_handle_t *uart, const uint8_t *data, size_t len) {
    if (uart == NULL || !uart->initialized || data == NULL || len == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    if (!uart->config.enable_dma) {
        printf("[UART%d] ERROR: DMA not enabled\n", uart->uart_id);
        return RTOS_ERROR_NOT_READY;
    }
    
    if (uart->tx_busy) {
        printf("[UART%d] ERROR: DMA transfer already in progress\n", uart->uart_id);
        return RTOS_ERROR_BUSY;
    }
    
    uart->tx_busy = true;
    
    printf("[UART%d] DMA TX: %zu bytes - ", uart->uart_id, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
    
    /*
     * REAL HARDWARE DMA CONFIGURATION:
     * 
     * // Set memory address (source)
     * DMA1_Stream6->M0AR = (uint32_t)data;
     * 
     * // Set number of data items to transfer
     * DMA1_Stream6->NDTR = len;
     * 
     * // Clear DMA flags
     * DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | 
     *               DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 |
     *               DMA_HIFCR_CFEIF6;
     * 
     * // Enable DMA stream
     * DMA1_Stream6->CR |= DMA_SxCR_EN;
     * 
     * // DMA will now transfer data to UART automatically
     * // When complete, DMA interrupt (DMA1_Stream6_IRQHandler) will fire
     * 
     * void DMA1_Stream6_IRQHandler(void) {
     *     if (DMA1->HISR & DMA_HISR_TCIF6) {  // Transfer complete
     *         // Clear flag
     *         DMA1->HIFCR = DMA_HIFCR_CTCIF6;
     *         
     *         // Disable DMA stream
     *         DMA1_Stream6->CR &= ~DMA_SxCR_EN;
     *         
     *         // Mark transfer as complete
     *         uart->tx_busy = false;
     *         
     *         // Call user callback
     *         if (uart->tx_callback) {
     *             uart->tx_callback(uart, uart->tx_callback_data);
     *         }
     *     }
     * }
     */
    
    /* Simulate DMA transfer completion */
    #ifdef _WIN32
    Sleep(len / 10 + 1);  // Simulate transfer time
    #endif
    
    uart->tx_busy = false;
    
    printf("[UART%d] DMA TX complete\n", uart->uart_id);
    
    return RTOS_OK;
}

/* 
 * Get number of bytes available in RX buffer
 * -------------------------------------------
 */
size_t uart_available(uart_handle_t *uart) {
    if (uart == NULL || !uart->initialized) {
        return 0;
    }
    
    return circ_buf_available(&uart->rx_buffer);
}

/* 
 * Flush UART buffers
 * ------------------
 * Clears both RX and TX buffers.
 */
rtos_status_t uart_flush(uart_handle_t *uart) {
    if (uart == NULL || !uart->initialized) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    circ_buf_flush(&uart->rx_buffer);
    circ_buf_flush(&uart->tx_buffer);
    
    printf("[UART%d] Buffers flushed\n", uart->uart_id);
    
    return RTOS_OK;
}

/* 
 * Set RX callback function
 * ------------------------
 * Registers a callback to be called when data is received.
 */
rtos_status_t uart_set_callback(uart_handle_t *uart, uart_callback_t callback, void *user_data) {
    if (uart == NULL || !uart->initialized) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    uart->rx_callback = callback;
    uart->rx_callback_data = user_data;
    
    printf("[UART%d] RX callback registered\n", uart->uart_id);
    
    return RTOS_OK;
}

/* 
 * UART Interrupt Handler (Simulation)
 * ------------------------------------
 * In real hardware, this would be the ISR that handles UART interrupts.
 */
void uart_irq_handler(uint8_t uart_id) {
    if (uart_id >= MAX_UART_INSTANCES) {
        return;
    }
    
    uart_handle_t *uart = &uart_instances[uart_id];
    
    if (!uart->initialized) {
        return;
    }
    
    /*
     * REAL UART INTERRUPT HANDLER (ARM Cortex-M):
     * 
     * void USART2_IRQHandler(void) {
     *     uint32_t sr = USART2->SR;  // Read status register
     *     
     *     // --- RX NOT EMPTY INTERRUPT ---
     *     if (sr & USART_SR_RXNE) {
     *         // Read received data
     *         uint8_t data = USART2->DR;  // Reading DR clears RXNE flag
     *         
     *         // Put in circular buffer
     *         if (!circ_buf_put(&uart->rx_buffer, data)) {
     *             // Buffer overflow - handle error
     *             // Could set error flag, drop data, etc.
     *         }
     *         
     *         // Call user callback
     *         if (uart->rx_callback) {
     *             uart->rx_callback(uart, uart->rx_callback_data);
     *         }
     *     }
     *     
     *     // --- TX EMPTY INTERRUPT ---
     *     if (sr & USART_SR_TXE) {
     *         uint8_t data;
     *         
     *         // Get next byte from TX buffer
     *         if (circ_buf_get(&uart->tx_buffer, &data)) {
     *             // Write to data register
     *             USART2->DR = data;  // Writing DR clears TXE flag
     *         } else {
     *             // No more data to send, disable TXE interrupt
     *             USART2->CR1 &= ~USART_CR1_TXEIE;
     *             
     *             // Call TX complete callback
     *             if (uart->tx_callback) {
     *                 uart->tx_callback(uart, uart->tx_callback_data);
     *             }
     *         }
     *     }
     *     
     *     // --- ERROR HANDLING ---
     *     if (sr & USART_SR_ORE) {  // Overrun error
     *         // Clear by reading SR and DR
     *         (void)USART2->SR;
     *         (void)USART2->DR;
     *     }
     *     
     *     if (sr & USART_SR_FE) {   // Framing error
     *         // Clear by reading SR and DR
     *         (void)USART2->SR;
     *         (void)USART2->DR;
     *     }
     *     
     *     if (sr & USART_SR_PE) {   // Parity error
     *         // Clear by reading SR and DR
     *         (void)USART2->SR;
     *         (void)USART2->DR;
     *     }
     * }
     * 
     * INTERRUPT PRIORITIES:
     * - Higher number = lower priority
     * - UART typically: priority 5-7 (medium)
     * - Critical tasks: priority 0-2 (high)
     * - Background tasks: priority 8-15 (low)
     */
    
    printf("[UART%d] IRQ Handler executed (simulated)\n", uart_id);
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
 * Called by device_init_all() when a UART device is being initialized.
 * 
 * @param dev Device structure with platform_data containing uart_config_t*
 * @return DEVICE_STATUS_OK on success
 */
device_status_t driver_uart_init(device_t *dev) {
    if (dev == NULL) {
        return DEVICE_STATUS_INVALID_PARAM;
    }
    
    printf("[UART Driver] Initializing device: %s (ID: %d)\n", dev->name, dev->id);
    
    /* platform_data should contain a pointer to uart_config_t */
    if (dev->platform_data == NULL) {
        printf("[UART Driver] ERROR: No platform_data provided\n");
        return DEVICE_STATUS_INVALID_PARAM;
    }
    
    uart_config_t *config = (uart_config_t *)dev->platform_data;
    
    /* Initialize the UART using existing uart_init() */
    uart_handle_t *uart_handle = uart_init(dev->id, config);
    
    if (uart_handle == NULL) {
        printf("[UART Driver] ERROR: uart_init() failed\n");
        return DEVICE_STATUS_ERROR;
    }
    
    /* Store the handle in driver_data for later retrieval */
    dev->driver_data = (void *)uart_handle;
    
    printf("[UART Driver] Device %s initialized successfully\n", dev->name);
    return DEVICE_STATUS_OK;
}