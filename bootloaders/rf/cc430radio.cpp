#include "cc430radio.h"

/**
 * Macros
 */
// Enter Tx state
#define setTxState()              Strobe(RF_STX)
// Enter IDLE state
#define setIdleState()            Strobe(RF_SIDLE)
// Flush Rx FIFO
#define flushRxFifo()             Strobe(RF_SFRX)
// Flush Tx FIFO
#define flushTxFifo()             Strobe(RF_SFTX)

/**
 * CC430RADIO
 * 
 * Class constructor
 */

// TODO: prova a modificare PA_LowPower con LongJohnson
CC430RADIO::CC430RADIO(void)
{
  carrierFreq = CFREQ_868;
  channel = CCDEF_CHANNR;
  syncWord[0] = CCDEF_SYNC0;
  syncWord[1] = CCDEF_SYNC1;
  devAddress = CCDEF_ADDR;
  paTableByte = PA_LongDistance;            // Priority = Long distance
//  paTableByte = PA_LowPower;            // Priority = Low power
}

/**
 * setCCregs
 * 
 * Configure CC430 radio registers
 */
void CC430RADIO::setCCregs(void)
{
  WriteSingleReg(IOCFG2, CCDEF_IOCFG2);
  WriteSingleReg(IOCFG0,  CCDEF_IOCFG0);
  WriteSingleReg(FIFOTHR,  CCDEF_FIFOTHR);
  WriteSingleReg(PKTLEN,  CCDEF_PKTLEN);
  WriteSingleReg(PKTCTRL1,  CCDEF_PKTCTRL1);
  WriteSingleReg(PKTCTRL0,  CCDEF_PKTCTRL0);

  // Set default synchronization word
  setSyncWord(syncWord);

  // Set default device address
  setDevAddress(devAddress);

  // Set default frequency channel
  setChannel(channel);

  WriteSingleReg(FSCTRL1,  CCDEF_FSCTRL1);
  WriteSingleReg(FSCTRL0,  CCDEF_FSCTRL0);

  // Set default carrier frequency = 868 MHz
  setCarrierFreq(carrierFreq);

  // RF speed
  if (workMode == MODE_4800)
    WriteSingleReg(MDMCFG4,  CCDEF_MDMCFG4_4800);
  else
    WriteSingleReg(MDMCFG4,  CCDEF_MDMCFG4_38400);

  WriteSingleReg(MDMCFG3,  CCDEF_MDMCFG3);
  WriteSingleReg(MDMCFG2,  CCDEF_MDMCFG2);
  WriteSingleReg(MDMCFG1,  CCDEF_MDMCFG1);
  WriteSingleReg(MDMCFG0,  CCDEF_MDMCFG0);
  WriteSingleReg(DEVIATN,  CCDEF_DEVIATN);
  WriteSingleReg(MCSM0,  CCDEF_MCSM0);
  WriteSingleReg(FOCCFG,  CCDEF_FOCCFG);
  WriteSingleReg(BSCFG,  CCDEF_BSCFG);
  WriteSingleReg(AGCCTRL2,  CCDEF_AGCCTRL2);
  WriteSingleReg(AGCCTRL1,  CCDEF_AGCCTRL2);
  WriteSingleReg(AGCCTRL0,  CCDEF_AGCCTRL2);
  WriteSingleReg(FREND1,  CCDEF_FREND1);
  WriteSingleReg(FREND0,  CCDEF_FREND0);
  WriteSingleReg(FSCAL3,  CCDEF_FSCAL3);
  WriteSingleReg(FSCAL2,  CCDEF_FSCAL2);
  WriteSingleReg(FSCAL1,  CCDEF_FSCAL1);
  WriteSingleReg(FSCAL0,  CCDEF_FSCAL0);
  WriteSingleReg(FSTEST,  CCDEF_FSTEST);
  WriteSingleReg(TEST1,  CCDEF_TEST1);
  WriteSingleReg(TEST0,  CCDEF_TEST0);

  enableCCA();
}

/**
 * sendData
 *
 * Send data packet via RF
 *
 * @param packet Packet to be transmitted. First byte is the destination address
 *
 * @return True if the transmission succeeds. False otherwise
 */
bool CC430RADIO::sendData(CCPACKET packet)
{
  bool res = false;
  uint8_t marcState;
  uint16_t count;

  MRFI_CLEAR_SYNC_PIN_INT_FLAG();
  MRFI_CLEAR_GDO0_INT_FLAG();

  // Disable Rx and enter in IDLE state
  setRxOffState();

  // Enter RX state again
  setRxState();

  // Check that the RX state has been entered
  while (((marcState = ReadSingleReg(MARCSTATE)) & 0x1F) != 0x0D)
  {
    if (marcState == 0x11)        // RX_OVERFLOW
      flushRxFifo();              // flush receive queue
  }

  delayMicroseconds(500);

  // Set data length at the first position of the TX FIFO
  WriteSingleReg(RF_TXFIFOWR,  packet.length);
  // Write data into the TX FIFO
  WriteBurstReg(RF_TXFIFOWR, packet.data, packet.length);

  MRFI_CLEAR_GDO0_INT_FLAG();

  // Transmit
  setTxState();

  // Check that TX state is being entered (state = RXTX_SETTLING)
  marcState = ReadSingleReg(MARCSTATE) & 0x1F;
  if((marcState != 0x13) && (marcState != 0x14) && (marcState != 0x15))
  {
    setIdleState();       // Enter IDLE state
    flushTxFifo();        // Flush Tx FIFO
    setRxState();         // Back to RX state
    return false;
  }

  delayMicroseconds(250);
  count = 0xFFFF;
  // Wait until packet transmission
  while(!MRFI_GDO0_INT_FLAG_IS_SET() && count--);

  if (!count)
  {
    setIdleState();       // Enter IDLE state
    flushTxFifo();        // Flush Tx FIFO
    res = false;
  }
    // Check that the TX FIFO is empty
  else if((ReadSingleReg(TXBYTES) & 0x7F) == 0)
    res = true;

  // Clear interrupt flags
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();
  MRFI_CLEAR_GDO0_INT_FLAG();

  // Enter back into RX state
  setRxOnState();

  return res;
}

/**
 * receiveData
 * 
 * Read data packet from RX FIFO
 *
 * @param packet Container for the packet received
 * 
 * @return Amount of bytes received
 */
uint8_t CC430RADIO::receiveData(CCPACKET *packet)
{
  uint8_t i;
  uint8_t rxBuffer[CCPACKET_BUFFER_LEN];
  uint8_t rxLength = ReadSingleReg(RXBYTES);

  // Any byte waiting to be read and no overflow?
  if ((rxLength & 0x7F) && !(rxLength & 0x80))
  {
    // If packet is too long
    if (rxLength > CCPACKET_BUFFER_LEN)
      packet->length = 0;   // Discard packet
    else
    {
      // Read data packet
      ReadBurstReg(RF_RXFIFORD, rxBuffer, rxLength);

      packet->length = rxBuffer[0];

      for(i=0 ; i<packet->length; i++)
        packet->data[i] = rxBuffer[i+1];

      // Read RSSI
      packet->rssi = rxBuffer[++i];
      // Read LQI and CRC_OK
      packet->lqi = rxBuffer[++i] & 0x7F;
      packet->crc_ok = rxBuffer[i] >> 7;
    }
  }
  else
    packet->length = 0;

  setIdleState();       // Enter IDLE state
  flushRxFifo();        // Flush Rx FIFO

  // Back to RX state
  setRxState();

  return packet->length;
}
