//
// Created by Alessandro Verlato on 16/11/22.
//

#include "functions.h"
#include "utils.h"

#define MIN_LINE_LEN 22

GWAP gwap;
// Global packet
CCPACKET packet;
CC430FLASH flash;
uint8_t receivedLines[MAX_SKETCH_LINES / 8]; // We can support max (8 * MAX_SKETCH_LINES) lines of code for the sketch

uint8_t calculateCrc(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;

  for (uint8_t i = 0; i < len; i++) {
    crc += data[i];
  }

  return crc;
}

bool checkCRC(uint8_t *data, uint8_t len) {
  uint8_t crc = calculateCrc(data, len - 1);

  if (crc == data[len - 1]) {
    return true;
  }

  return false;
}

bool checkForFactoryReset() {
  uint16_t counter = 0;
  while (IS_RESET_PIN_LOW()) {
    if (counter >= 500) {
      return true;
    }
    delayMicroseconds(50000);
    counter++;
  }
  return false;
}

uint8_t* createQueryDataFrom(uint8_t *buf, uint16_t fwVer, uint16_t lineNum, uint16_t capab) {
  buf[0] = (fwVer >> 8) & 0xFF;
  buf[1] = fwVer & 0xFF;
  buf[2] = (lineNum >> 8) & 0xFF;
  buf[3] = lineNum & 0xFF;
  buf[4] = (capab >> 8) & 0xFF;
  buf[5] = capab & 0xFF;

  return buf;
}

void delayClockCycles(register uint32_t n) {
  __asm__ __volatile__ (
          "1: \n"
          " dec        %[n] \n"
          " jne        1b \n"
          :[n] "+r"(n));
}

void directJump() {
  // Read user code starting address
  uint16_t *ptr2 = (uint16_t*) USER_RESET_VECTOR;
  uint16_t userCodeAddr = *ptr2;

  // if (userCodeAddr == 0xA000) {
  //   for (uint8_t j = 0; j < 20; j++) {
  //     LED_ON();
  //     delayClockCycles(50000);
  //     LED_OFF();
  //     delayClockCycles(50000);
  //   }
  // }

  // __asm__ __volatile__ (
  //       "push 0xA000 \n"
  //       "ret");


  //  __asm__ (
  //       " BR #0xA000"
  // );

  //  __asm__ (
  //       " mov.w %0, r14          ; load user's reset_vector addr\n"
  //       " mov.w @r14, r15        ; load the value at that vector\n"
  //       " cmp.w #0xffff,r15      ; is there and addr there? or just erased bytes\n"
  //       " jeq   run_gdb_boot     ; if erased, don't launch just start bootloader\n"
  //       " mov.w @r14, r0         ; else jump to usercode, never return r0==PC\n"
  //       "run_gdb_boot:           ;\n"
  //       :
  //       : "i" (USER_RESET_VECTOR)
  //       : "cc"
  //     );  

  void (*p)(void);
  p = (void (*)(void)) userCodeAddr;
  (*p)();
}

void eraseUROM(bool eraseINFO) {
  // Point at the begining of user flash
  uint16_t currentUROMSegment = USER_CODE_STARTING_ADDR;

  LED_ON();
  while (currentUROMSegment < VECTOR_TABLE_SEGMENT) {
    flash.eraseSegment((uint8_t *) currentUROMSegment);
    currentUROMSegment += FLASH_SEGMENT_SIZE;
  }

  if (eraseINFO) flash.eraseSegment((uint8_t *) INFOMEM_CONFIG);
  LED_OFF();
}

void factoryReset() {
  // Erase UROM and INFO memory
  eraseUROM(true);
}

uint16_t getCapabilities() {
  uint16_t capabilities = 0;
  capabilities |= CAPABILITY_2LINES;
  return capabilities;
}

uint16_t getFwVersion(uint8_t *line) {
  uint16_t version = ((uint16_t)line[GWAP_DATA_HEAD_LEN] << 8) | line[GWAP_DATA_HEAD_LEN + 1];

  return version;
}

uint16_t getLineNumber(uint8_t *data) {
  uint16_t lineNb = data[0];
  lineNb <<= 8;
  lineNb |= data[1];

  return lineNb;
}

uint16_t getTargetAddress(uint8_t *line) {
  uint16_t address = line[0];
  address <<= 8;
  address |= line[1];

  return address;
}

// Return @true if the line has already been flashed
bool hasLineBeenFlashed(uint16_t lineNumber) {
  // Bounds-check the OTA line number: a malformed packet could pass a line
  // beyond MAX_SKETCH_LINES and index receivedLines[] out of bounds.
  if (lineNumber >= MAX_SKETCH_LINES) return false;
  uint16_t index = lineNumber / 8;
  uint8_t mask = 1 << (lineNumber % 8);
  return (receivedLines[index] & mask) != 0;
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



   // POWER: Turn ADC and reference voltage off to conserve power
  ADC12CTL0 &= ~ADC12ENC;
  ADC12CTL0 &= ~ADC12ON;
  ADC12CTL0 &= ~ADC12REFON;
  REFCTL0 &= ~REFON;
  REFCTL0 |= REFTCOFF;  // Temp sensor disabled
  
  // Config pins as outputs by default except P2, wich contains the ADC inputs
  P1DIR = 0xFF;
  P3DIR = 0xFF;
  PJDIR = 0xFF;
  
  // Config pins as outputs by default except P2, wich contains the ADC inputs
//  P1DIR = 0xFF;
//  P3DIR = 0xFF;
//  PJDIR = 0xFF;
}

void jumpToUserCode() {
  // triggerBOR();
 directJump();
}

// Mark a line as already flashed
void markLineAsFlashed(uint16_t lineNumber) {
  // Bounds-check: ignore an out-of-range line so a malformed packet can't write
  // past receivedLines[].
  if (lineNumber >= MAX_SKETCH_LINES) return;
  uint16_t index = lineNumber / 8;
  uint8_t mask = 1 << (lineNumber % 8);
  receivedLines[index] |= mask;
}

// void markLineAsRequired(uint16_t lineNumber) {
//   uint16_t index = lineNumber / 8;
//   uint8_t mask = 0 << (lineNumber % 8);
//   receivedLines[index] &= mask;
// }

// void fixFailedLine(uint16_t receivedLineAddr, uint16_t userCodeAddr) {
//   // Calculate segment to erase addr
//   uint16_t segmentAddr = userCodeAddr + ((receivedLineAddr / 0x200) * 0x200);
//   // Erase segment
//   flash.eraseSegment((uint8_t *) segmentAddr);

//   // Mark segment lines as not written
//   // uint16_t startingIndex = 0;
//   // uint16_t endingIndex = 0;  
// }

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

uint32_t random(uint32_t min_num, uint32_t max_num) {
  return rand() % (max_num + 1 - min_num) + min_num;
}

bool readHexLine() {
  // Any packet waiting to be read?
  if (gwap.radio.receiveData(&packet) > 0) {
    if (packet.length >= MIN_LINE_LEN) {
      // Is CRC OK?
      if (packet.crc_ok) {
        // Function
        if ((packet.GWAP_FUNCTION) == GWAPFUNCT_STA) {
          // Break if packet pcode != our pcode
          if (!gwap.hasCCPACKETMyProductCode(&packet)) return false;
          // Firmware page received?
          if (packet.GWAP_REGID == REGI_FWVERSION) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

bool transmitGwapQueryLine(uint8_t *data) {
  return gwap.sendPacket((uint8_t)GWAPFUNCT_QRY, (uint8_t)REGI_FWVERSION, data, 6);
}

void triggerBOR() {
  // Exit upgrade mode
  uint8_t state = (uint8_t) SYSTATE_RESTART;
  TRANSMIT_GWAP_STATUS_STATE(state);

  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMSWBOR;
  PMMCTL0_H = 0x00;

  while(1);
}
