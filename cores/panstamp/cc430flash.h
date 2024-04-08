/**
 * Copyright (c) 2014 panStamp <contact@panstamp.com>
 * 
 * This file is part of the panStamp project.
 * 
 * panStamp  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 * 
 * panStamp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with panStamp; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 
 * USA
 * 
 * Author: Daniel Berenguer
 * Creation date: 10/23/2013
 */

#ifndef _CC430FLASH_H
#define _CC430FLASH_H

#include "wiring.h"

class CC430FLASH
{
  public:
    /**
     * read
     * 
     * Read buffer from info memory
     *
     * @param buffer pointer to the buffer where to write the result
     * @param section info memory section (memory address)
     * @param position position in section
     * @pararm length Length to be read
     *
     * @return amount of bytes read
     */
    virtual uint8_t read(uint8_t *buffer, uint16_t section, uint16_t position, uint16_t length) { return read(buffer, section, position, length, 512); }
    uint8_t read(uint8_t *buffer, uint16_t section, uint16_t position, uint8_t length, uint16_t size);

    /**
     * write
     * 
     * Write buffer in info memory
     *
     * @param buffer array to be written
     * @param section info memory section (memory address)
     * @param position position in section
     * @pararm length Length to be written
     *
     * @return amount of bytes copied
     */
    uint8_t write(uint8_t *buffer, uint16_t section, uint16_t position, uint8_t length);

    void eraseSegment(uint8_t *memAddress);
};

#endif
