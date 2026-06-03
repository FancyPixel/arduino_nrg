#ifndef _CC430FLASH_H_
#define _CC430FLASH_H_

#include <cc430f5137.h>
#include "datatypes.h"


class CC430FLASH
{
  private:
  
  /**
   * waitReady
   * 
   * Wait until the flash memory is ready for the next operation
   */
  ALWAYS_INLINE
	void waitReady()
  {
  	while(FCTL3 & BUSY);
	}

  public:
  
  /**
   * eraseSegment
   * 
   * Erase flash segment
   * 
   * @param memAddress starting address of the flash segment to be erased
   */
  NEVER_INLINE
  void eraseSegment(uint8_t *memAddress)
  {
	  waitReady();
    FCTL3 = FWKEY;              // Clear LOCK
    FCTL1 = FWKEY | ERASE;      // Enable segment erase
    *memAddress = 0;            // Dummy write, erase Segment
  	waitReady();
    FCTL3 = FWKEY | LOCK;       // Done, set LOCK
  }

  /**
   * read
   * 
   * Red contents from flash memory
   *
   * @param memAddress starting address in flash memory
   * @param buffer array to copy the contents into
   * @param length Length in bytes to be read
   *
   * @return amount of bytes read
   */
  NEVER_INLINE
  uint8_t read(uint8_t *memAddress, uint8_t *buffer, uint8_t length)
  {
    uint8_t i;
    uint8_t * flashPtr = memAddress;
                                          
    for (i = 0; i < length; i++)
      buffer[i] = *flashPtr++;           // Read byte from flash

    return length;
  }

  /**
   * write
   * 
   * Write buffer in info memory
   *
   * @param memAddress destination address in flash memory
   * @param buffer array to be written. This buffer must contain the whole contents of the
                   flash section to be written
   * @param length Length to be written
   *
   * @return amount of bytes copied
   */
  NEVER_INLINE

  uint8_t write(uint8_t *memAddress, uint8_t *buffer, uint8_t length)
  {
    uint16_t i = 0;
                                          
    waitReady();
    FCTL3 = FWKEY;                         // Clear Lock bit
    FCTL1 = FWKEY+WRT;                     // Set WRT bit for byte write operation

    while (i < length)
    {
      *memAddress = buffer[i];             // Write byte in flash
      
      waitReady();                         // Wait for write to complete
      
      if (*memAddress == buffer[i])        // Check flash contents before skipping to the next position
      {
        memAddress++;
        i++;
      }
    }

    waitReady();
    FCTL1 = FWKEY;                         // Clear WRT bit
    FCTL3 = FWKEY+LOCK;                    // Set LOCK bit

    return length;
  }

  uint8_t update(uint8_t *buffer, uint16_t section, uint16_t position, uint16_t length) {
    if ((position + length) > 512) {
      return 0;                           // out of range
    }

    uint8_t buf[512];
    uint16_t i, j;
    char * flashPtr = (char *) section;   // Initialize Flash pointer

    for (i = 0; i < 512; i++) {
      buf[i] = flashPtr[i];                // Save current contents in temporary buffer
    }

    FCTL3 = FWKEY;

    FCTL1 = FWKEY+ERASE;                   // Set Erase bit
    *flashPtr = 0;                         // Dummy write to erase Flash seg
    waitReady();                           // Wait for erase to finish before writing
    FCTL1 = FWKEY+WRT;                     // Set WRT bit for byte write operation

    for (i = 0; i < 512; i++) {
      if (i == position) {
        for (j = 0; j < length; j += 2) {
          *flashPtr++ = buffer[j+1];         // Write byte to flash
          *flashPtr++ = buffer[j];         // Write byte to flash
        }

        i += length-1;
      } else {
        *flashPtr++ = buf[i];              // Write byte to flash
      }
    }

    FCTL1 = FWKEY;                         // Clear WRT bit
    FCTL3 = FWKEY+LOCK;                    // Set LOCK bit

    return length;
  }
};

#endif
