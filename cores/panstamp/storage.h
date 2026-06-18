#ifndef STORAGE_h
#define STORAGE_h

#include "cc430info.h"

#define DEFAULT_NVOLAT_SECTION  INFOMEM_SECTION_D

#define STORAGE  CC430INFO
#define FLASH CC430FLASH

/**
 * RAM memory
 */
const uint16_t RAM_END_ADDRESS = 0x2BFF;

/**
 * Flash memory
 */
const uint16_t WIRELESS_BOOT_ADDR = 0x8000;

const uint16_t USER_STARTING_ADDR = 0xFFBC;

const uint16_t VECTOR_TABLE_ADDR = 0xFF80;

// Whole 512-byte segment containing the vector table (0xFE00 - 0xFFFF).
// flash.update() and the OTA finalization erase + rewrite this segment.
// If interrupted (power loss, glitch), the segment is left in an inconsistent
// state: BSL password (0xFFE0-0xFFFF) and reset vectors get corrupted.
const uint16_t VECTOR_TABLE_SEGMENT = 0xFE00;

// Backup copy of the vector table, used to recover from corruption.
// The bootloader validates the main vector table at boot and restores
// from this backup if it detects inconsistency.
// Located in the highest free flash segment (firmware ends at ~0xEBED).
const uint16_t VECTOR_TABLE_BACKUP_SEGMENT = 0xFC00;

#endif

