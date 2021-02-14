/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2013 Andrzej Surowiec <emeryth@gmail.com>
 * Copyright (C) 2013 Pavol Rusnak <stick@gk2.sk>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ramdisk.h"

#include <stdio.h>
#include <string.h>

#include "libopencm3/cm3/scb.h"
#include "libopencm3/stm32/flash.h"
#include "libopencm3/stm32/desig.h"
#include "libopencm3/stm32/rcc.h"
#include "libopencm3/cm3/systick.h"
#include "libopencmsis/core_cm3.h"

#include "target_flash.h"
#include "uf2.h"

#define WBVAL(x) ((x) & 0xFF), (((x) >> 8) & 0xFF)
#define QBVAL(x) ((x) & 0xFF), (((x) >> 8) & 0xFF),\
         (((x) >> 16) & 0xFF), (((x) >> 24) & 0xFF)

// filesystem size is 5MB (10240 * SECTOR_SIZE)
#define SECTOR_COUNT              (2 * 1024 * 3)
#define SECTOR_SIZE               BYTES_PER_SECTOR
#define SECTORS_PER_CLUSTER       1
#define RESERVED_SECTORS          1
#define SECTOR_PER_FAT            24
#define FAT_COPIES                2
#define ROOT_ENTRIES              16
#define ROOT_ENTRY_LENGTH         32
#define FILEDATA_START_CLUSTER    2
#define START_SECTOR_FAT          1
#define START_SECTOR_ROOT         (START_SECTOR_FAT + SECTOR_PER_FAT * FAT_COPIES)
#define START_SECTOR_DATA         (START_SECTOR_ROOT + 1)


#define ENTRY_ATTR_READONLY      (1 << 0)
#define ENTRY_ATTR_HIDDEN        (1 << 1)
#define ENTRY_ATTR_SYSTEM        (1 << 2)
#define ENTRY_ATTR_VOL_LABEL     (1 << 3)
#define ENTRY_ATTR_SUBDIR        (1 << 4)
#define ENTRY_ATTR_ARCHIVE       (1 << 5)

uint8_t boot_sector[] = {
    0xEB, 0x3C, 0x90,                           // code to jump to the bootstrap code
    'm', 'k', 'd', 'o', 's', 'f', 's', 0x00,    // OEM ID
    WBVAL(BYTES_PER_SECTOR),                    // bytes per sector
    SECTORS_PER_CLUSTER,                        // sectors per cluster
    WBVAL(RESERVED_SECTORS),                    // # of reserved sectors (1 boot sector)
    FAT_COPIES,                                 // FAT copies (2)
    WBVAL(ROOT_ENTRIES),                        // root entries (16)
    WBVAL(SECTOR_COUNT),                        // total number of sectors
    0xF8,                                       // media descriptor (0xF8 = Fixed disk)
    WBVAL(SECTOR_PER_FAT),                      // sectors per FAT (1)
    0x32, 0x00,                                 // sectors per track (32)
    0x64, 0x00,                                 // number of heads (64)
    0x00, 0x00, 0x00, 0x00,                     // hidden sectors (0)
    0x00, 0x00, 0x00, 0x00,                     // large number of sectors (0)
    0x80,                                       // drive number (0)
    0x00,                                       // reserved
    0x29,                                       // extended boot signature
    0x69, 0x17, 0xAD, 0x53,                     // volume serial number
    'R', 'A', 'M', 'D', 'I', 'S', 'K', ' ', ' ', ' ', ' ',   // volume label
    'F', 'A', 'T', '1', '6', ' ', ' ', ' '                   // filesystem type
};

typedef struct {
    char     name[8];
    char     ext[3];
    uint8_t  attrs;
    uint8_t  reserve;
    uint8_t  create_time_refine;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t high_start_cluster;
    uint16_t update_time;
    uint16_t update_date;
    uint16_t start_cluster;
    uint32_t size;
} __packed dir_entry_t;


uint16_t *flash_size;

const char infoUf2File[] =
    "UF2 Bootloader 1.0.0\r\n"
    "Model: STM32F1\r\n"
    "Board-ID: STM32F1-Dummy\r\n";


dir_entry_t root_dir[] = {
    {
        .name = "STM32F1 ",
        .ext = "   ",
        .attrs = ENTRY_ATTR_VOL_LABEL,  // 卷标名
    },
    {
        .name = "INFO_UF2",
        .ext = "TXT",
        .attrs = ENTRY_ATTR_ARCHIVE,
        .start_cluster = 2,
        .size = sizeof(infoUf2File),
    },
    {
        .name = "FLASH   ",
        .ext = "BIN",
        .attrs = ENTRY_ATTR_ARCHIVE,
        .start_cluster = 3,
    },
};

extern uint8_t app_valid;


int ramdisk_init(void)
{
    root_dir[2].size = desig_get_flash_size() * 1024;

    return 0;
}

int ramdisk_read(uint32_t lba, uint8_t *copy_to)
{
    memset(copy_to, 0, SECTOR_SIZE);
    if (lba < START_SECTOR_FAT) {
        /* Boot 信息 */
        memcpy(copy_to, boot_sector, sizeof(boot_sector));
        copy_to[SECTOR_SIZE - 2] = 0x55;
        copy_to[SECTOR_SIZE - 1] = 0xAA;
    } else if (lba < START_SECTOR_ROOT) {
        /* 生成 FAT 表 */
        uint16_t cluster_index;
        uint16_t *fat_to = (uint16_t *)copy_to;

        lba -= START_SECTOR_FAT;
        lba %= SECTOR_PER_FAT;
        cluster_index = lba * (SECTOR_SIZE / 2);

        if (lba == 0) {
            fat_to[(cluster_index++) % (SECTOR_SIZE / 2)] = 0xFFF8;
            fat_to[(cluster_index++) % (SECTOR_SIZE / 2)] = 0xFFFF;
        }
        for (uint32_t i = 1; i < sizeof(root_dir) / sizeof(dir_entry_t); i++) {
            uint16_t cluster_start = root_dir[i].start_cluster;
            uint16_t cluster_end = cluster_start + (root_dir[i].size / (SECTORS_PER_CLUSTER * SECTOR_SIZE));

            while (cluster_start <= cluster_index && cluster_index <= cluster_end) {
                    fat_to[cluster_index % (SECTOR_SIZE / 2)] = (cluster_index == cluster_end ? 0xFFFF : cluster_index + 1);
                    if (cluster_index++ % (SECTOR_SIZE / 2) == 255) {
                        return 512;
                    }
            }
        }
    } else if (lba < START_SECTOR_DATA) {
        /* Root directory */
        for (uint32_t i = 0; i < sizeof(root_dir) / sizeof(dir_entry_t); i++) {
            memcpy(copy_to + i * sizeof(dir_entry_t), root_dir + i, sizeof(dir_entry_t));
        }
    } else if (lba < SECTOR_COUNT) {
        /* File data */
        lba -= START_SECTOR_DATA;

        /* INFO_UF2.TXT */
        if (lba == 0) {
            memcpy(copy_to, infoUf2File, root_dir[1].size);
        }
        /* FLASH.BIN */
        else {
            lba -= 1;
            if ((uint32_t)lba * SECTOR_SIZE < root_dir[2].size) {
            memcpy(copy_to, (void *)((uint32_t)0x08000000 + lba * SECTOR_SIZE), SECTOR_SIZE);
            }
        }
    }
    return lba < SECTOR_COUNT ? 512 : -1;
}

static inline uint8_t is_uf2_block(uf2_block_t const *bl)
{
  return (bl->magic_start_0 == UF2_MAGIC_START0) &&
         (bl->magic_strat_1 == UF2_MAGIC_START1) &&
         (bl->magic_end == UF2_MAGIC_END) &&
         !(bl->flags & UF2_FLAG_NOFLASH);
}

int ramdisk_write(uint32_t lba, const uint8_t *copy_from)
{
    (void) lba;
    uf2_block_t *bl = (void *)copy_from;

    if (!is_uf2_block(bl)) {
        return -1;
    }

    target_flash_program(bl->target_addr, bl->data, bl->payload_size);

    if (bl->block_no == bl->num_blocks - 1) {
       app_valid = 1;
    }
    return 0;
}

int ramdisk_blocks(void)
{
    return SECTOR_COUNT;
}
