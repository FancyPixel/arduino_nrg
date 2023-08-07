#ifndef _GWAP_H
#define _GWAP_H

#include <string.h>
#include "cc430radio.h"
#include "memconfig.h"
#include "product.h"
#include "cc430flash.h"
//#include "utils.h"

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
    SYSTATE_UPGRADE,
    SYSTATE_STARTUP
};


/**
 * GWAP PACKET definitions
 */
#define GWAP_FUNCTION           data[13]
#define GWAP_REGID              data[14]
#define GWAP_PRODUCT_ID_LEN     8
#define GWAP_PRODUCT_CODE_LEN   4
#define GWAP_ADDRESS_LEN        (GWAP_PRODUCT_ID_LEN + GWAP_PRODUCT_CODE_LEN)  // 12
#define GWAP_DATA_HEAD_LEN      (GWAP_ADDRESS_LEN + 3)  // 15
#define GWAP_REG_VAL_LEN        (CC1101_DATA_LEN - GWAP_DATA_HEAD_LEN)    // SWAP data payload - max length
#define GWAP_NB_TX_TRIES        3                                         // Number of transmission retries
#define GWAP_TX_DELAY           10                                        // Delay before sending (msec)
#define GWAP_POS_NONCE          GWAP_ADDRESS_LEN                          // Position of nonce in GWAP packet
#define GWAP_POS_FUNCTION       (GWAP_ADDRESS_LEN + 1)                    // Position of function code in GWAP packet
#define GWAP_POS_REGID          (GWAP_ADDRESS_LEN + 2)                    // Position of register ID in GWAP packet
#define CRC_LEN                 1

class GWAP {
public:

    CC430RADIO radio;

    // GWAP address
    uint8_t devAddress[GWAP_ADDRESS_LEN];
    // Address is composed of productId + productCode (appended)
    uint8_t productId[GWAP_PRODUCT_ID_LEN];
    uint8_t productCode[GWAP_PRODUCT_CODE_LEN];

    ALWAYS_INLINE
    void init(uint8_t freq = CFREQ_868, uint8_t mode = MODE_38400) {
      uint8_t i, tmp[8];

      // Build UID
      getUID(tmp);
      // Extract PRODUCT_ID
      for (i = 0; i < sizeof(tmp); i++) {
        devAddress[i] = tmp[sizeof(tmp) - 1 - i];
      }

      // *** Product code is GWAP_PRODUCT_CODE ***

      // Build mote address by appending PRODUCT_CODE to PRODUCT_ID
      uint8_t len = sizeof(GWAP_PRODUCT_CODE);
      for (i = 0; i < len; i++) {
        devAddress[i + GWAP_ADDRESS_LEN - len] = GWAP_PRODUCT_CODE[i];
      }

      // Config radio settings
      radio.devAddress = devAddress[0];
      radio.syncWord[0] = CCDEF_SYNC0;
      radio.syncWord[1] = CCDEF_SYNC1;
      // Init radio module
      radio.init(freq, mode);
    }

    // Check if provided CCPACKET is addressed to this mote (same complete address of 12 bytes)
    bool isCCPACKETAddressedToMe(CCPACKET *packet, bool allowBroadcast = false) {
      uint8_t packetAddr[GWAP_ADDRESS_LEN];
      uint8_t clonedAddr[GWAP_ADDRESS_LEN];
      memcpy(clonedAddr, devAddress, sizeof(devAddress));

      // Extract packet address
      for (uint8_t i = 0; i < GWAP_ADDRESS_LEN; i++) {
        packetAddr[i] = packet->data[i];
      }

      bool exatchMatch = memcmp(packetAddr, devAddress, sizeof(packetAddr)) == 0;
      // If broadcast is allowed, skip first byte check
      if (allowBroadcast) clonedAddr[0] = 0;
      bool broadcastMatch = memcmp(packetAddr, clonedAddr, sizeof(packetAddr)) == 0;

      return exatchMatch || broadcastMatch;
    }

    // Check if provided CCPACKET has the same product code of this mote
    bool hasCCPACKETMyProductCode(CCPACKET *packet) {
      uint8_t packetProductCode[GWAP_PRODUCT_CODE_LEN];

      for (uint8_t i = 0; i < GWAP_PRODUCT_CODE_LEN; i++) {
        packetProductCode[i] = packet->data[GWAP_PRODUCT_ID_LEN + i];
      }

      return memcmp(packetProductCode, GWAP_PRODUCT_CODE, sizeof(packetProductCode)) == 0;
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
     *
     * @return True if the transmission succeeds. False otherwise
     */
    bool sendPacket(uint8_t funct, uint8_t regId, uint8_t *val, uint8_t len);

    /**
     * sendPacketVal
     *
     * Send GWAP packet. Accept different types as data payload
     *
     * @param funct Function code
     * @param regId Register ID
     * @param val   Register value
     *
     * @return True if the transmission succeeds. False otherwise
     */
    template<class T>
    bool sendPacketVal(uint8_t funct, uint8_t regId, T val) {
      int i;
      int size = sizeof(val);
      uint8_t buf[size];

      for (i = size; i > 0; i--) {
//        flashMorseLine(val & 0xFF);
        buf[i - 1] = val & 0xFF;
        val >>= 8;
      }

      return sendPacket(funct, regId, buf, size);
    }

    void nvolatToFactoryDefaults() {
      CC430FLASH nvMem;

      // Signature
      uint8_t signature[] = {NVOLAT_SIGNATURE_HIGH, NVOLAT_SIGNATURE_LOW};
      nvMem.write((uint8_t *) NVOLAT_SIGNATURE, signature, sizeof(signature));

      // Frequency channel
      uint8_t channel[] = {CCDEF_CHANNR};
      nvMem.write((uint8_t *) NVOLAT_FREQ_CHANNEL, channel, sizeof(channel));

      // Sync word
      uint8_t syncW[] = {CCDEF_SYNC1, CCDEF_SYNC0};
      nvMem.write((uint8_t *) NVOLAT_SYNC_WORD, syncW, sizeof(syncW));

      // TX interval
      uint8_t txInt[] = {0xFF, 0};
      nvMem.write((uint8_t *) NVOLAT_TX_INTERVAL, txInt, sizeof(txInt));
    }
};

#endif
