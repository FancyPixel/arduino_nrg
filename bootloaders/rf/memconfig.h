/*
 * memconfig.h - 
 *
 *  Created: Apr 11, 2013
 *   Author: rick@kimballsoftware.com
 *     Date: 04-11-2013
 *  Version: 1.0.1
 *
 *  09-18-2013 rkimball - added configuration for cc4305137
 *
 * =========================================================================
 *  Copyright © 2013 Rick Kimball
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MEMCONFIG_H_
#define MEMCONFIG_H_

#include "datatypes.h"
#include "codesize.h"
#include <math.h>

/*
 * These are chip specific values that indicate the start
 * of user flash and the start of our gdb_bootloader code
 *
 * Look at the linker map, you will have to adjust based on the
 * size of this code. These values are for a cc430f5137
 *
 * Make sure the values here match the ones in gdb_memory.x linker file
 *
 */
 
/**
 * RAM memory
 */
const uint16_t RAM_END_ADDRESS   = 0x2BFF;
/**
 * INFO MEMORY
 */
const uint16_t INFOMEM_SECTIO_A  = 0x1980;
const uint16_t INFOMEM_SECTIO_B  = 0x1900;
const uint16_t INFOMEM_SECTIO_C  = 0x1880;
const uint16_t INFOMEM_SECTIO_D  = 0x1800;

#define INFOMEM_CONFIG  INFOMEM_SECTIO_D

#define NVOLAT_SIGNATURE      0x00   // 2-byte register
#define NVOLAT_FREQ_CHANNEL   0x02   // 1-byte register
#define NVOLAT_SYNC_WORD      0x03   // 2-byte register
#define NVOLAT_TX_INTERVAL    0x05   // 2-byte register
#define NVOLAT_FIRST_CUSTOM   0x20

/**
 * Standard flash
 */

const uint16_t VECTOR_TABLE_SEGMENT   = 0xFE00;     // Flash segment address containing isr vectors
// Backup copy of the vector table — written before any destructive op on
// VECTOR_TABLE_SEGMENT. If main vector table is corrupted at boot
// (power loss during update), the bootloader restores from this copy.
const uint16_t VECTOR_TABLE_BACKUP_SEGMENT = 0xFC00; // Highest free flash segment (firmware ends ~0xEBED)
const uint16_t VECTOR_TABLE_ADDR      = 0xFF80;     // Starting address containing isr vectors
const uint16_t USER_RESET_VECTOR      = 0xFFBC;     // Flash location that stores user's reset vector
const uint16_t FACTORY_RESET_VECTOR   = 0xFFBA;     // Flash location that stores factory reset vector
const uint16_t BL_VERSION_H_VECTOR    = 0xFFB6;     // Flash location that stores Bootloader version vector (high word)
const uint16_t BL_VERSION_L_VECTOR    = 0xFFB8;     // Flash location that stores Bootloader version vector (low word)
const uint16_t GDB_BOOT_RESET_VECTOR  = 0xFFFE;     // MSP430's reset vector

const uint16_t FLASH_SEGMENT_SIZE           = 0x200;    // 512 bytes is the size of each flash segment
const uint16_t BOOTLOADER_STARTING_ADDR     = 0x8000;   // Bootloader starting address - matches main memory starting addr. See "CC430F5137_memory_organization.png" side here
const uint16_t USER_CODE_STARTING_ADDR      = ceil((float)(BOOTLOADER_STARTING_ADDR + BOOTLOADER_CODE_SIZE) / (float)FLASH_SEGMENT_SIZE) * FLASH_SEGMENT_SIZE;   // Flash starting address for user code

#endif /* MEMCONFIG_H_ */
