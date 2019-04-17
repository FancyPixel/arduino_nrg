#ifndef STORAGE_h
#define STORAGE_h

#include "cc430info.h"

#define DEFAULT_NVOLAT_SECTION  INFOMEM_SECTION_D

#define STORAGE  CC430INFO

/**
 * RAM memory
 */
const uint16_t RAM_END_ADDRESS = 0x2BFF;

/**
 * Flash memory
 */
const uint16_t WIRELESS_BOOT_ADDR = 0x8000;

#endif

