//
// Created by Alessandro Verlato on 16/11/22.
//

#include "functions.h"

GWAP gwap;
// Global packet
CCPACKET packet;
bool isVirgin = false;
uint8_t receivedLines[MAX_SKETCH_LINES / 8]; // We can support max (8 * MAX_SKETCH_LINES) lines of code for the sketch


bool readHexLine() {
  // Any packet waiting to be read?
  if (gwap.radio.receiveData(&packet) > 0) {
    // Is CRC OK?
    if (packet.crc_ok) {
      // Function
      if ((packet.GWAP_FUNCTION) == GWAPFUNCT_STA) {
        // Break if packet pcode == our pcode
        if (!gwap.hasCCPACKETMyProductCode(&packet)) return false;
        // Firmware page received?
        if (packet.GWAP_REGID == REGI_FWVERSION) {
          return true;
        }
      }
    }
  }

  return false;
}

void initCore() {
  // Configure PMM
  SetVCore(2);

  // Set the High-Power Mode Request Enable bit so LPM3 can be entered
  // with active radio enabled
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L;
  PMMCTL0_H = 0x00;

  /**
   * Enable 32kHz ACLK
   */
  P5SEL |= 0x03;                      // Select XIN, XOUT on P5.0 and P5.1
  UCSCTL6 &= ~XT1OFF;                  // XT1 On, Highest drive strength
  UCSCTL6 |= XCAP_3;                  // Internal load cap

  /*
   * Select XT1 as FLL reference
   */
  UCSCTL3 = SELA__XT1CLK;
  UCSCTL4 = SELA__XT1CLK | SELS__DCOCLKDIV | SELM__DCOCLKDIV;

  /**
   * Configure CPU clock for 12MHz
   */
  _BIS_SR(SCG0);              // Disable the FLL control loop
  UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx
  UCSCTL1 = DCORSEL_5;        // Select suitable range
  UCSCTL2 = FLLD_1 + 0x16E;   // Set DCO Multiplier
  _BIC_SR(SCG0);              // Enable the FLL control loop

  // Worst-case settling time for the DCO when the DCO range bits have been
  // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
  // UG for optimization.
  // 32 x 32 x 8 MHz / 32,768 Hz = 250000 = MCLK cycles for DCO to settle
  delayClockCycles(250000L);

  // Loop until XT1 & DCO stabilizes, use do-while to ensure that
  // the body is executed at least once
  do {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
    SFRIFG1 &= ~OFIFG;                      // Clear fault flags
  } while ((SFRIFG1 & OFIFG));

  UCSCTL6 &= ~(XT1DRIVE_3);                 // Xtal is now stable, reduce drive
  // strength

  /*
   * Select Interrupt edge for PA_PD and SYNC signal:
   * Interrupt Edge select register: 1 == Interrupt on High to Low transition.
   */
  RF1AIES = BIT0 | BIT9;

  // Config pins as outputs by default except P2, wich contains the ADC inputs
  P1DIR = 0xFF;
  P3DIR = 0xFF;
  PJDIR = 0xFF;
}

// Return @true if the line has already been flashed
bool hasLineBeenFlashed(uint16_t lineNumber) {
  uint16_t index = lineNumber / 8;
  uint8_t mask = 1 << (lineNumber % 8);
  return (receivedLines[index] & mask) != 0;
}

// Mark a line as already flashed
void markLineAsFlashed(uint16_t lineNumber) {
  uint16_t index = lineNumber / 8;
  uint8_t mask = 1 << (lineNumber % 8);
  receivedLines[index] |= mask;
}

// Return the first not-already-written line number
uint16_t nextNeededLineNumber() {
  uint16_t index, i;
  uint8_t mask;

  for (i = 0; i < MAX_SKETCH_LINES; i++) {
    index = i / 8;
    mask = 1 << (i % 8);
    if ((receivedLines[index] & mask) == 0) return i;
  }

  // A fallback that should never be reached
  return MAX_SKETCH_LINES;
}

void factoryReset() {
  CC430FLASH nvMem;

  // Erase info memory
  nvMem.eraseSegment((uint8_t *) INFOMEM_CONFIG);

  for (int i = 0; i < 6; i++) {
    LED_ON();
    delayClockCycles(1000000L);
    LED_OFF();
    delayClockCycles(1000000L);
  }

  isVirgin = true;
}

uint16_t getLineNumber(uint8_t *data) {
  uint16_t lineNb = data[2];
  lineNb <<= 8;
  lineNb |= data[3];

  return lineNb;
}

uint16_t getTargetAddress(uint8_t *line) {
  uint16_t address = line[0];
  address <<= 8;
  address |= line[1];

  return address;
}

uint16_t getFwVersion(uint8_t *line) {
  uint16_t version = ((uint16_t)line[GWAP_DATA_HEAD_LEN] << 8) | line[GWAP_DATA_HEAD_LEN + 1];

  return version;
}

bool checkCRC(uint8_t *data, uint8_t len) {
  uint8_t crc = len - 4;
  uint8_t i, dataLen = len - 1;

  for (i = 0; i < dataLen; i++) {
    crc += data[i];
  }

  crc = ~crc + 1;

  if (crc == data[dataLen]) {
    return true;
  }

  return false;
}

void jumpToUserCode() {
  // Exit upgrade mode
  uint8_t state = (uint8_t) SYSTATE_RESTART;
  TRANSMIT_GWAP_STATUS_STATE(state);

  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMSWBOR;
  PMMCTL0_H = 0x00;

  while(1);
}

void delayClockCycles(register uint32_t n) {
  __asm__ __volatile__ (
          "1: \n"
          " dec        %[n] \n"
          " jne        1b \n"
          :[n] "+r"(n));
}

uint32_t random(uint32_t min_num, uint32_t max_num) {
  return rand() % (max_num + 1 - min_num) + min_num;
}

void ledBlink(uint8_t times) {
  register uint32_t cycles = 200000000L;
  uint8_t delayTimes = 2;

  uint8_t i, j;
  for (i = 0; i < times; i++) {
    LED_ON();
    for (j = 0; j < delayTimes; j++) { delayClockCycles(cycles); }
    LED_OFF();
    for (j = 0; j < delayTimes; j++) { delayClockCycles(cycles); }
  }
}


uint16_t getCapabilities() {
  uint16_t capabilities = 0;
  capabilities |= CAPABILITY_2LINES;
  return capabilities;
}

uint8_t* createQueryDataFrom(uint16_t firmwareVersion, uint16_t lineNumber, uint16_t capabilities) {
  uint8_t buf[] = {
          (firmwareVersion >> 8) & 0xFF, firmwareVersion & 0xFF,
          (lineNumber >> 8) & 0xFF, lineNumber & 0xFF,
          (capabilities >> 8) & 0xFF, capabilities & 0xFF
  };

  return buf;
}

bool transmitGwapQueryLine(uint8_t *data) {
  gwap.sendPacket((uint8_t)GWAPFUNCT_QRY, (uint8_t)REGI_FWVERSION, data, 6);  // TODO:  len??!?!?
}
