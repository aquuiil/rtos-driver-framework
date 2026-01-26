#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "rtos_types.h"

/* UART configuration */
typedef enum {
    UART_BAUD_9600 = 9600,
    UART_BAUD_19200 = 19200,
    UART_BAUD_38400 = 38400,
    UART_BAUD_57600 = 57600,
    UART_BAUD_115200 = 115200,
    UART_BAUD_230400 = 230400,
    UART_BAUD_460800 = 460800,
    UART_BAUD_921600 = 921600
} uart_baud_t;

typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
} uart_parity_t;

typedef enum {
    UART_STOPBITS_1 = 0,
    UART_STOPBITS_2
} uart_stopbits_t;

typedef enum {
    UART_DATABITS_7 = 7,
    UART_DATABITS_8 = 8,
    UART_DATABITS_9 = 9
} uart_databits_t;

typedef struct {
    uart_baud_t baud_rate;
    uart_parity_t parity;
    uart_stopbits_t stop_bits;
    uart_databits_t data_bits;
    bool enable_dma;
    bool enable_interrupts;
    uint16_t rx_buffer_size;
    uint16_t tx_buffer_size;
} uart_config_t;

/* UART handle */
typedef struct uart_handle uart_handle_t;

/* UART callback function type */
typedef void (*uart_callback_t)(uart_handle_t *uart, void *user_data);

/* UART API */
uart_handle_t* uart_init(uint8_t uart_id, const uart_config_t *config);
rtos_status_t uart_deinit(uart_handle_t *uart);
rtos_status_t uart_write(uart_handle_t *uart, const uint8_t *data, size_t len);
rtos_status_t uart_read(uart_handle_t *uart, uint8_t *buffer, size_t len, size_t *bytes_read);
rtos_status_t uart_write_dma(uart_handle_t *uart, const uint8_t *data, size_t len);
size_t uart_available(uart_handle_t *uart);
rtos_status_t uart_flush(uart_handle_t *uart);
rtos_status_t uart_set_callback(uart_handle_t *uart, uart_callback_t callback, void *user_data);

/* UART interrupt simulation */
void uart_irq_handler(uint8_t uart_id);

#endif /* UART_DRIVER_H */