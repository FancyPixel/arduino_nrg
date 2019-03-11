#include "gwap.h"


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
void GWAP::sendPacket(uint8_t funct, uint8_t regId, uint8_t *val, uint8_t len)
{
  uint8_t i;
  CCPACKET packet;
  static uint8_t nonce = 0;
  uint8_t tmpCrc = 0;

  packet.length = GWAP_DATA_HEAD_LEN + len + CRC_LEN;

  // TUT IN ZIR
  for(i = 0; i < GWAP_ADDRESS_LEN; i++) {
    packet.data[i] = devAddress[i];
  }

  packet.data[12] = nonce++;
  packet.data[13] = funct;
  packet.data[14] = regId;

  for (i = 0 ; i < len ; i++) {
    packet.data[GWAP_DATA_HEAD_LEN + i] = val[i];
  }

  for(i = 0; i < sizeof(packet.data) - 1; i++) {
    tmpCrc += packet.data[i];
  }

  packet.data[GWAP_DATA_HEAD_LEN + len] = tmpCrc;

  radio.sendData(packet);
}
