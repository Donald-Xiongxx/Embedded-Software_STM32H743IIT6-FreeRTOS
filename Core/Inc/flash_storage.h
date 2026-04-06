#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>

int flash_storage_init(void);
uint32_t flash_storage_get_start(void);
uint32_t flash_storage_get_size(void);
int flash_storage_read(uint32_t offset, uint8_t *buf, uint32_t len);
int flash_storage_write(uint32_t offset, const uint8_t *buf, uint32_t len);

#endif
