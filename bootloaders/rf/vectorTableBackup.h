/*
 * vectorTableBackup.h
 *
 * Backup/validate/restore of the vector table segment (0xFE00-0xFFFF).
 *
 * Why:
 *   The vector table contains ISR vectors, the bootloader reset vector
 *   (0xFFFE), the user reset vector (0xFFBC), the bootloader version
 *   (0xFFB6) and the BSL password (0xFFE0-0xFFFF). The bootloader and
 *   panstamp.goToWirelessBoot() must erase and rewrite this whole 512-byte
 *   segment to update one of these fields. If power is lost mid-rewrite,
 *   the segment ends up partially written: BSL password corrupted (no
 *   recovery via BSL possible), reset vectors potentially garbage.
 *
 *   To survive this, we keep a backup copy of the vector table in another
 *   flash segment (VECTOR_TABLE_BACKUP_SEGMENT = 0xFC00). The backup is
 *   written BEFORE any destructive operation on the main segment. At boot,
 *   the bootloader validates the main segment and, if corrupted, restores
 *   from the backup.
 *
 * Validation:
 *   The reset vector at 0xFFFE is written LAST during a vector table
 *   rewrite. If it equals BOOTLOADER_STARTING_ADDR (0x8000), the rewrite
 *   completed successfully. Any other value (especially 0xFFFF after a
 *   partial erase) indicates corruption.
 */

#ifndef VECTOR_TABLE_BACKUP_H
#define VECTOR_TABLE_BACKUP_H

#include "memconfig.h"
#include "cc430flash.h"
#include "product.h"

extern CC430FLASH flash;

/**
 * isVectorTableValid (MAIN segment)
 *
 * Returns true if the main segment at 0xFE00 contains a complete, consistent
 * vector table. Two checks:
 *
 *  1. Reset vector at offset 0x1FE (= 0xFFFE) must point to the bootloader
 *     (BOOTLOADER_STARTING_ADDR = 0x8000).
 *
 *  2. Bootloader version at 0x1B6/0x1B8 (= 0xFFB6/0xFFB8) must match the
 *     version compiled into this bootloader (FIRMWARE_VERSION). The bootloader
 *     stamps this field during a rewrite, so a mismatch means the segment was
 *     left inconsistent (interrupted rewrite, or freshly downloaded firmware
 *     VT not yet stamped) and should be restored from the backup.
 *
 * Layout of the BL version bytes in the segment:
 *     isrTable[3][0x06] = FIRMWARE_VERSION[1]   -> 0xFFB6
 *     isrTable[3][0x07] = FIRMWARE_VERSION[0]   -> 0xFFB7
 *     isrTable[3][0x08] = FIRMWARE_VERSION[3]   -> 0xFFB8
 *     isrTable[3][0x09] = FIRMWARE_VERSION[2]   -> 0xFFB9
 * So the little-endian uint16_t at 0xFFB6 is (FIRMWARE_VERSION[0]<<8 | FIRMWARE_VERSION[1])
 * and the one at 0xFFB8 is (FIRMWARE_VERSION[2]<<8 | FIRMWARE_VERSION[3]).
 */
static inline bool isVectorTableValid() {
    uint16_t base = VECTOR_TABLE_SEGMENT;
    uint16_t resetVec = *(uint16_t*)(base + 0x1FE);
    if (resetVec != BOOTLOADER_STARTING_ADDR) return false;

    uint16_t blVerH = *(uint16_t*)(base + 0x1B6);
    uint16_t blVerL = *(uint16_t*)(base + 0x1B8);
    uint16_t expectedH = ((uint16_t)FIRMWARE_VERSION[0] << 8) | FIRMWARE_VERSION[1];
    uint16_t expectedL = ((uint16_t)FIRMWARE_VERSION[2] << 8) | FIRMWARE_VERSION[3];
    if (blVerH != expectedH) return false;
    if (blVerL != expectedL) return false;

    return true;
}

/**
 * isVectorTableBackupValid (BACKUP segment)
 *
 * The backup is "good enough to restore from" if restoring it produces a
 * bootable, BSL-recoverable main segment. That requires:
 *
 *  1. Reset vector == bootloader address, so after the restore the MCU boots
 *     the bootloader, which then re-stamps the BL version through the normal
 *     update path (rfloader.cpp BL version check).
 *
 *  2. A non-erased BSL password, so BSL recovery remains possible.
 *
 * The BL version is intentionally NOT checked here. The backup is always
 * captured BEFORE the BL version is stamped (see backupVectorTable call
 * sites: the version-update path backs up the pre-update segment, and the OTA
 * finalization backs up the just-downloaded firmware VT which has no BL
 * version yet). So a valid backup legitimately holds 0xFFFF there.
 */
static inline bool isVectorTableBackupValid() {
    uint16_t base = VECTOR_TABLE_BACKUP_SEGMENT;
    uint16_t resetVec = *(uint16_t*)(base + 0x1FE);
    if (resetVec != BOOTLOADER_STARTING_ADDR) return false;

    // BSL password occupies 0x1E0-0x1FF. Check the bytes before the reset
    // vector (0x1E0-0x1FD): if all 0xFF the backup was never populated.
    for (uint16_t off = 0x1E0; off < 0x1FE; off++) {
        if (*(uint8_t*)(base + off) != 0xFF) return true;
    }
    return false;
}

/**
 * copyFlashSegment
 *
 * Copy 512 bytes from src segment to dst segment. dst is erased first.
 * Used both for backup (main -> backup) and restore (backup -> main).
 */
static inline void copyFlashSegment(uint16_t srcAddr, uint16_t dstAddr) {
    uint8_t buf[FLASH_SEGMENT_SIZE];
    uint8_t* src = (uint8_t*)srcAddr;
    for (uint16_t i = 0; i < FLASH_SEGMENT_SIZE; i++) buf[i] = src[i];

    flash.eraseSegment((uint8_t*)dstAddr);

    // The bootloader's flash.write has uint8_t length (max 255), so we split
    // the 512-byte payload into chunks of at most 200 bytes.
    flash.write((uint8_t*)(dstAddr +   0), buf +   0, 200);
    flash.write((uint8_t*)(dstAddr + 200), buf + 200, 200);
    flash.write((uint8_t*)(dstAddr + 400), buf + 400, 112);
}

/**
 * backupVectorTable
 *
 * Save current vector table (0xFE00-0xFFFF) into the backup segment.
 * Call this BEFORE any erase/rewrite of the vector table segment.
 */
static inline void backupVectorTable() {
    copyFlashSegment(VECTOR_TABLE_SEGMENT, VECTOR_TABLE_BACKUP_SEGMENT);
}

/**
 * restoreVectorTable
 *
 * Recover the vector table from the backup. Call only when isVectorTableValid
 * returns false AND isVectorTableBackupValid returns true.
 *
 * The backup holds BL version 0xFFFF (it is captured before the version is
 * stamped). We stamp the correct BL version into the RAM buffer BEFORE writing,
 * so the restored main is immediately valid (isVectorTableValid true on the
 * next boot) and we avoid a restore -> BOR -> restore loop. This is done in a
 * SINGLE erase+write cycle (no follow-up flash.update) to minimize the window
 * during which the segment — including the BSL password — is inconsistent.
 *
 * BL version byte layout (matches OTA finalization / flash.update):
 *   buf[0x1B6] = FW[1] -> 0xFFB6      buf[0x1B8] = FW[3] -> 0xFFB8
 *   buf[0x1B7] = FW[0] -> 0xFFB7      buf[0x1B9] = FW[2] -> 0xFFB9
 *
 * After restore, the caller should triggerBOR() to reboot cleanly.
 */
static inline void restoreVectorTable() {
    uint8_t buf[FLASH_SEGMENT_SIZE];
    uint8_t* src = (uint8_t*)VECTOR_TABLE_BACKUP_SEGMENT;
    for (uint16_t i = 0; i < FLASH_SEGMENT_SIZE; i++) buf[i] = src[i];

    buf[0x1B6] = FIRMWARE_VERSION[1];
    buf[0x1B7] = FIRMWARE_VERSION[0];
    buf[0x1B8] = FIRMWARE_VERSION[3];
    buf[0x1B9] = FIRMWARE_VERSION[2];

    flash.eraseSegment((uint8_t*)VECTOR_TABLE_SEGMENT);
    flash.write((uint8_t*)(VECTOR_TABLE_SEGMENT +   0), buf +   0, 200);
    flash.write((uint8_t*)(VECTOR_TABLE_SEGMENT + 200), buf + 200, 200);
    flash.write((uint8_t*)(VECTOR_TABLE_SEGMENT + 400), buf + 400, 112);
}

#endif /* VECTOR_TABLE_BACKUP_H */
