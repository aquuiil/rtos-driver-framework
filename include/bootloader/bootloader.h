#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "rtos_types.h"

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t crc32;
    uint32_t entry_point;
} firmware_header_t;

void bootloader_init(void);
rtos_status_t bootloader_verify_image(const firmware_header_t *header);
void bootloader_jump_to_app(uint32_t app_address);

#endif