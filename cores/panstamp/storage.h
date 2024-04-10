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

#endif

