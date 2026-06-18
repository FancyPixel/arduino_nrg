#ifndef _PRODUCT_H
#define _PRODUCT_H

/**
 * Hardware version
 */
const uint8_t HARDWARE_VERSION[] = { 0, 0, 0, 1 };

/**
 * Firmware version
 *
 * Kept at v5 on purpose: the cc430info/nvolatToFactoryDefaults hardening
 * added on top of v5 (see cc430info.cpp and gwap.h, part of v20 software
 * hardening) is delivered by BSL re-flash only, not via bootloader OTA.
 * Bumping the version here would make the concentrator auto-trigger a
 * bootloader OTA on every bollard reporting blVersion=5, which is a
 * high-risk operation we do not want to run automatically.
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
