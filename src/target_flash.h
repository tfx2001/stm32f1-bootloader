#if !defined(__TARGET_FLASH_H)
#define __TARGET_FLASH_H

#include <stdint.h>

#include "libopencm3/stm32/flash.h"

#define FLASH_SIZE          (256U * 1024U)
#define FLASH_SECTOR_SIZE   2048U
#define FLASH_SECTOR_COUNT  (FLASH_SIZE / FLASH_SECTOR_SIZE)
#define APPLICATION_OFFSET  0x4000U                             // reserve 16KB for bootloader
#define APPLICATION_ENTRY   (FLASH_BASE + APPLICATION_OFFSET)

void target_flash_program(uint32_t dst, uint8_t *src, uint32_t len);

#endif // __TARGET_FLASH_H
