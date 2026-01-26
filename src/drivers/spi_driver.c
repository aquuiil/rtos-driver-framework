#include "drivers/spi_driver.h"
#include <stdio.h>
#include <string.h>

struct spi_handle {
    uint8_t spi_id;
    spi_config_t config;
    bool initialized;
    volatile bool busy;
};

#define MAX_SPI_INSTANCES 4
static spi_handle_t spi_instances[MAX_SPI_INSTANCES] = {0};

spi_handle_t* spi_init(uint8_t spi_id, const spi_config_t *config) {
    if (spi_id >= MAX_SPI_INSTANCES || config == NULL) {
        return NULL;
    }
    
    spi_handle_t *spi = &spi_instances[spi_id];
    if (spi->initialized) return spi;
    
    memset(spi, 0, sizeof(spi_handle_t));
    spi->spi_id = spi_id;
    spi->config = *config;
    spi->initialized = true;
    
    printf("[SPI%d] Initialized: Mode=%d, Speed=%d, DMA=%s\n",
           spi_id, config->mode, config->speed, 
           config->enable_dma ? "ON" : "OFF");
    
    /* Real HW: Configure SPI registers, GPIO, DMA, interrupts */
    return spi;
}

rtos_status_t spi_deinit(spi_handle_t *spi) {
    if (!spi || !spi->initialized) return RTOS_ERROR_INVALID_PARAM;
    spi->initialized = false;
    printf("[SPI%d] Deinitialized\n", spi->spi_id);
    return RTOS_OK;
}

rtos_status_t spi_transfer(spi_handle_t *spi, const uint8_t *tx_data, uint8_t *rx_data, size_t len) {
    if (!spi || !spi->initialized || !tx_data || !rx_data || len == 0) {
        return RTOS_ERROR_INVALID_PARAM;
    }
    
    printf("[SPI%d] TRANSFER (%zu bytes)\n  TX: ", spi->spi_id, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", tx_data[i]);
        rx_data[i] = tx_data[i] ^ 0xFF;  // Simulate loopback
    }
    printf("\n  RX: ");
    for (size_t i = 0; i < len; i++) printf("%02X ", rx_data[i]);
    printf("\n");
    
    /* Real HW: 
     * for (i = 0; i < len; i++) {
     *     while (!(SPI1->SR & SPI_SR_TXE));
     *     SPI1->DR = tx_data[i];
     *     while (!(SPI1->SR & SPI_SR_RXNE));
     *     rx_data[i] = SPI1->DR;
     * }
     */
    return RTOS_OK;
}

rtos_status_t spi_write(spi_handle_t *spi, const uint8_t *data, size_t len) {
    uint8_t dummy_rx[256];
    return spi_transfer(spi, data, dummy_rx, len);
}

rtos_status_t spi_read(spi_handle_t *spi, uint8_t *buffer, size_t len) {
    uint8_t dummy_tx[256] = {0xFF};
    return spi_transfer(spi, dummy_tx, buffer, len);
}