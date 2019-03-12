/**
 * Copyright (c) 2016 panStamp S.L.U. <contact@panstamp.com>
 *
 * This file is part of the panStamp project.
 *
 * panStamp  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * panStamp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with panStamp; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 *
 * Author: Daniel Berenguer
 * Creation date: 03/11/2016
 */

#ifndef _PRODUCT_H
#define _PRODUCT_H

/**
 * Hardware version
 */
//#define HARDWARE_VERSION        0x00000300L
const uint8_t HARDWARE_VERSION[] = { 0, 0, 3, 0 };

/**
 * Firmware version
 */
//#define FIRMWARE_VERSION        0x00000100L
const uint8_t FIRMWARE_VERSION[] = { 0, 0, 1, 0 };

/**
 * Product code
 */
//#define GWAP_PRODUCT_CODE       0x00010004L
const uint8_t GWAP_PRODUCT_CODE[] = { 0, 1, 0, 4 };

#endif
