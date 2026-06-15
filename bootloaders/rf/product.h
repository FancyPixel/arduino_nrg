#ifndef _PRODUCT_H
#define _PRODUCT_H

/**
 * Hardware version
 */
const uint8_t HARDWARE_VERSION[] = { 0, 0, 0, 1 };

/**
 * Firmware version
 *
 * v6 (2026-06): re-enable __disable_interrupt in CC430INFO::write, add
 *               waitReady() after erase, refactor nvolatToFactoryDefaults
 *               to a single atomic erase+write. See cc430info.cpp and
 *               gwap.h. Part of v20 software hardening.
 * v5 (2026-04): vector table backup/restore to survive power loss during
 *               OTA finalization, BL version update, and goToWirelessBoot.
 *               See vectorTableBackup.h.
 */
const uint8_t FIRMWARE_VERSION[] = { 0, 0, 0, 6 };

/**
 * Product code
 */
const uint8_t GWAP_PRODUCT_CODE[] = { 0, 0, 0, 2 };

#endif
