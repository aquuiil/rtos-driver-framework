#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include "rtos_types.h"

typedef enum {
    SPI_MODE_0 = 0,  // CPOL=0, CPHA=0
    SPI_MODE_1,      // CPOL=0, CPHA=1
    SPI_MODE_2,      // CPOL=1, CPHA=0
    SPI_MODE_3       // CPOL=1, CPHA=1
} spi_mode_t;

typedef enum {
    SPI_SPEED_LOW = 0,
    SPI_SPEED_MEDIUM,
    SPI_SPEED_HIGH,
    SPI_SPEED_VERY_HIGH
} spi_speed_t;

typedef struct {
    spi_mode_t mode;
    spi_speed_t speed;
    bool enable_dma;
} spi_config_t;

typedef struct spi_handle spi_handle_t;

spi_handle_t* spi_init(uint8_t spi_id, const spi_config_t *config);
rtos_status_t spi_deinit(spi_handle_t *spi);
rtos_status_t spi_transfer(spi_handle_t *spi, const uint8_t *tx_data, uint8_t *rx_data, size_t len);
rtos_status_t spi_write(spi_handle_t *spi, const uint8_t *data, size_t len);
rtos_status_t spi_read(spi_handle_t *spi, uint8_t *buffer, size_t len);

#endif /* SPI_DRIVER_H */