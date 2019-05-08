#ifndef _GWAP_H
#define _GWAP_H


#include "cc430radio.h"
#include "memconfig.h"
#include "product.h"
#include "utils.h"

/**
 * GWAP message functions
 */
enum GWAPFUNCT {
    GWAPFUNCT_STA = 0x00,
    GWAPFUNCT_QRY,
    GWAPFUNCT_CMD
};

/**
 * Register index
 */
enum REGISTERS {
    REGI_PRODUCTCODE = 0,
    REGI_HWVERSION,
    REGI_FWVERSION,
    REGI_SYSSTATE,
    REGI_FREQCHANNEL,
    REGI_NETWORKID,
    REGI_TXINTERVAL
};

/**
 * System states
 */
enum SYSTATE {
    SYSTATE_RESTART = 0,
    SYSTATE_RXON,
    SYSTATE_RXOFF,
    SYSTATE_SYNC,
    SYSTATE_LOWBAT,
    SYSTATE_UPGRADE
};

/**
 * GWAP PACKET definitions
 */
#define GWAP_FUNCTION           data[13]
#define GWAP_REGID              data[14]
#define GWAP_ADDRESS_LEN        12
#define GWAP_DATA_HEAD_LEN     (GWAP_ADDRESS_LEN + 3)
#define GWAP_REG_VAL_LEN       (CC1101_DATA_LEN - GWAP_DATA_HEAD_LEN)   // SWAP data payload - max length
#define GWAP_NB_TX_TRIES       3                                        // Number of transmission retries
#define GWAP_TX_DELAY          10                                       // Delay before sending (msec)
#define GWAP_POS_NONCE         GWAP_ADDRESS_LEN                      // Position of nonce in GWAP packet
#define GWAP_POS_FUNCTION      (GWAP_POS_NONCE + 1)                     // Position of function code in GWAP packet
#define GWAP_POS_REGID         (GWAP_POS_FUNCTION + 1)                  // Position of register ID in GWAP packet
#define CRC_LEN                1

#define GWAP_FUNCTION   data[13]
#define GWAP_REGID      data[14]

class GWAP {
public:

    /**
     * Radio object
     */
    CC430RADIO radio;

    /**
     * GWAP address
     */
    uint8_t devAddress[12];

    ALWAYS_INLINE
    void init(uint8_t freq = CFREQ_868, uint8_t mode = MODE_38400) {
      uint8_t buf[2];
      CC430FLASH flash;
      uint8_t i, tmp[8];

      // Build UID
      getUID(tmp);
      for (i = 0; i < sizeof(tmp); i++) {
        devAddress[i] = tmp[sizeof(tmp) - 1 - i];
      }

      // TODO get Product Code
      uint8_t len = sizeof(GWAP_PRODUCT_CODE);
      for (i = 0; i < len; i++) {
//        devAddress[i + GWAP_ADDRESS_LEN - len] = (GWAP_PRODUCT_CODE >> (8 * (len - 1 - i))) & 0xFF;
        devAddress[i + GWAP_ADDRESS_LEN - len] = GWAP_PRODUCT_CODE[i];
      }

      // Config radio settings
      radio.devAddress = devAddress[0];

      // Network ID
//      flash.read((uint8_t *)INFOMEM_SYNC_WORD, buf, 2);
//      if (buf[0] != 0xFF && buf[1] != 0xFF) {
      radio.syncWord[0] = CCDEF_SYNC0;
      radio.syncWord[1] = CCDEF_SYNC1;
//      }

      // Init radio module
      radio.init(freq, mode);
    }

    /**
     * getUID
     *
     * Read Die Record from Device Descriptor memory and build UID
     *
     * @param buffer Pointer to the buffer that will receive the result
     */
    inline void getUID(uint8_t *buffer) {
      uint8_t *flashPtr = (uint8_t *) 0x1A0A;
      buffer[0] = flashPtr[3]; // Wafer ID
      buffer[1] = flashPtr[2];
      buffer[2] = flashPtr[1];
      buffer[3] = flashPtr[0];
      buffer[6] = flashPtr[5]; // Die X position
      buffer[7] = flashPtr[4];
      buffer[4] = flashPtr[7]; // Die Y position
      buffer[5] = flashPtr[6];
    }

    /**
     * sendPacket
     *
     * Send GWAP packet
     *
     * @param funct Function code
     * @param regId Register ID
     * @param val   Register value
     * @param len   Register length
     */
    void sendPacket(uint8_t funct, uint8_t regId, uint8_t *val, uint8_t len);

    /**
     * sendPacketVal
     *
     * Send GWAP packet. Accept different types as data payload
     *
     * @param funct Function code
     * @param regId Register ID
     * @param val   Register value
     */
    template<class T>
    void sendPacketVal(uint8_t funct, uint8_t regId, T val) {
      int i;
      uint8_t buf[4];

      for (i = sizeof(val); i > 0; i--) {
        buf[i - 1] = val & 0xFF;
        val >>= 8;
      }

      sendPacket(funct, regId, buf, sizeof(val));
    }
};

#endif
