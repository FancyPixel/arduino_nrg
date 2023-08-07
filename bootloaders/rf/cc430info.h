#ifndef _CC430INFO_H
#define _CC430INFO_H

#include "wiring.h"

// Signature
#define NVOLAT_SIGNATURE_HIGH     0xAB
#define NVOLAT_SIGNATURE_LOW      0xCD


class CC430INFO
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
//    uint8_t read(uint8_t *buffer, uint16_t section, uint16_t position, uint8_t length);

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

//    void eraseSegment(uint8_t *memAddress);
};

#endif
