#include "target_flash.h"

static uint8_t erased_sectors[FLASH_SECTOR_COUNT];

static uint32_t flash_get_sector(uint32_t addr) {
    addr -= FLASH_BASE;
    return (addr / FLASH_SECTOR_SIZE);
}

void target_flash_program(uint32_t dst, uint8_t *src, uint32_t len) {
    uint32_t target_sector_begin;
    uint32_t target_sector_end;

    target_sector_begin = (dst - FLASH_BASE) / FLASH_SECTOR_SIZE;
    target_sector_end = (dst + len - FLASH_BASE) / FLASH_SECTOR_SIZE;

    /* 禁止写入 BootLoader 区域 */
    if (dst < APPLICATION_ENTRY) {
        return;
    }
    
    /* 判断是否已擦写 */
    for (uint32_t i = target_sector_begin; i <= target_sector_end; i++) {
        if (!erased_sectors[i]) {
            flash_unlock();
            flash_erase_page(FLASH_BASE + i * FLASH_SECTOR_SIZE);
            flash_wait_for_last_operation();
            flash_lock();
            /* 标记当前扇区已经擦写 */
            erased_sectors[i] = 1;
        }
    }

    /* 开始编程 */
    flash_unlock();
    for (uint32_t i = 0; i < len; i += 4) {
        flash_program_word(dst + i, *(uint32_t *)(src + i));
        flash_wait_for_last_operation();
    }
    flash_lock();
}