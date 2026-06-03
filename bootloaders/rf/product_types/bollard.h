#ifndef _PRODUCT_H
#define _PRODUCT_H

/**
 * Hardware version
 */
const uint8_t HARDWARE_VERSION[] = { 0, 0, 0, 1 };

/**
 * Firmware version
 *
 * v5 (2026-04): vector table backup/restore to survive power loss during
 *               OTA finalization, BL version update, and goToWirelessBoot.
 *               See vectorTableBackup.h.
 */
const uint8_t FIRMWARE_VERSION[] = { 0, 0, 0, 5 };

/**
 * Product code
 */
const uint8_t GWAP_PRODUCT_CODE[] = { 0, 0, 0, 2 };

#endif
