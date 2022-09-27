#include "rfloader.h"

//#define WORKING_MODE MODE_4800
#define WORKING_MODE MODE_38400

// Responses from server have to be received before 200 ms after sending the query
#define RESPONSE_TIMEOUT 500
#define MAX_SKETCH_LINES 1600
#define MAX_FAILED_LINE_REQUESTS 3

// Global packet
CCPACKET packet;
bool isVirgin = false;
uint8_t receivedLines[MAX_SKETCH_LINES / 8]; // We can support max (8 * MAX_SKETCH_LINES) lines of code for the sketch

ALWAYS_INLINE
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

int main(void) {
  bool firstLine = true;
  // Length of last line received
  uint8_t dataLineLength = 0;
  // Pointer to line buffer
  uint8_t *dataLine;
  // ISR vector table
  uint8_t isrTable[8][16];
  // User code address
  uint16_t userCodeAddr;
  bool correctLineReceived = false;
  uint16_t firmwareVersion = 0xFFFF;
  uint32_t fwVersionAndLineNumber = 0xFFFFFFFF;
  uint16_t _fwVersion = 0xFFFF; // This must be here
  uint8_t status, bytes, i;
  bool requestLine = true;
  uint16_t failedLineRequests = 0;
  uint16_t receivedLineNumber = 0;
  uint16_t nextLine;
  uint16_t lastLineNumber = -10; // Init a lot negative
  /*
   * *** IMPORTANT ***
   * DO NOT CHANGE the order of the code lines if you want have a working morse decoding
   */

  CONFIG_LED();
//  CONFIG_MORSE_OUT();
  CONFIG_RESET_PIN();

  // *** You must uncomment this line in order to startup morse decoding ***
  //  flashMorseString("start\n");

  // This flag will tell us whether wireless bootloading needs to start or not
  bool *ptr1;
  ptr1 = (bool *) RAM_END_ADDRESS;  // Memory address at the end of the stack
  bool runUserCode = *ptr1;       // Read value

  // Read user code starting address
  uint16_t *ptr2;
  ptr2 = (uint16_t *) USER_RESET_VECTOR;
  userCodeAddr = *ptr2;

  // Disable interrupts
  __disable_interrupt();

  // Check for factory reset
  uint32_t counter = 0;
  while (IS_RESET_PIN_LOW()) {
    if (counter >= 400000) {
      factoryReset();
      break;
    }
    counter++;
  }

  // Some fancy blinking for signaling that we're on bootloader
  for (int j = 0; j < 2; j++) {
    LED_ON();
    delayClockCycles(5000L);
    LED_OFF();
    delayClockCycles(5000L);
  }

  // Init core
  initCore();

  // Valid starting address of user code?
  if (!isVirgin && (userCodeAddr != 0xFFFF)) {
    // Jump to user code if the wireless bootloader was not called from there
    if (runUserCode) {
      jumpToUserCode();
    }
  }

  CC430FLASH flash;
  TIMER1A0 timer;

  // Init GWAP comms
  gwap.init(CFREQ_868, WORKING_MODE);

  // Pointer at the begining of user flash
  uint16_t address = USER_ROMADDR;

  while (1) {
    while (!correctLineReceived) {
      nextLine = nextNeededLineNumber();
      if (requestLine) {
        // Combine fwVersion and needed line number
        fwVersionAndLineNumber = (((uint32_t)firmwareVersion) << 16) | nextLine;
        LED_ON();
        // Query firmware line
        TRANSMIT_GWAP_QUERY_LINE(fwVersionAndLineNumber);
        LED_OFF();
        failedLineRequests++;
      }

      // Start timer
      timer.start(RESPONSE_TIMEOUT);

      while (!timer.timeout()) {
        status = ReadSingleReg(PKTSTATUS);
        bytes = ReadSingleReg(RXBYTES);

        // Poll PKSTATUS and number of bytes in the Rx FIFO
        if ((status & 0x01) && bytes) {
          while (ReadSingleReg(PKTSTATUS) & 0x01);
          // Packet received. Read packet and extract HEX line
          if (readHexLine()) {
            failedLineRequests = 0;
            // RF Packet OK: crc ok, function = status, product code ok, regId = firmware
            // Extract firmware version
            _fwVersion = getFwVersion(packet.data);
            // Data payload
            dataLine = packet.data + GWAP_DATA_HEAD_LEN;
            dataLineLength = packet.length - GWAP_DATA_HEAD_LEN - 1;

            // Correct data length?
            if (dataLineLength > BYTES_PER_LINE) {
              correctLineReceived = false;
              requestLine = false;
              break;
            }

            receivedLineNumber = getLineNumber(dataLine);

            if (firstLine) {
              // Skip line if it isn't the first one. Ask again line 0, with some probability
              if (receivedLineNumber != 0) {
                correctLineReceived = false;
                if (random(0, 100) < 66) {
                  requestLine = true;
                } else requestLine = false;
                break;
              }
              // Skip line if packet is not addressed to us
              if (!gwap.isCCPACKETAddressedToMe(&packet)) {
                correctLineReceived = false;
                requestLine = true;
                break;
              }
              // If everything is OK (first line received), track firmware version
              firmwareVersion = _fwVersion;
              correctLineReceived = true;
              requestLine = false; // Tag along and try to use rows addressed to others
              break;
            } else {
              // We already received a first line, so we know which firmware version we need (stored in firmwareVersion)
              // Break if we received a packet with a wrong firmware version
              if (_fwVersion != firmwareVersion) {
                requestLine = true;
                correctLineReceived = false;
                break;
              }
              // If we need the line (it hasn't already been flashed)
              if (!hasLineBeenFlashed(receivedLineNumber)) {
                if (gwap.isCCPACKETAddressedToMe(&packet, true)) {
                  // I'm a "Master" mote requesting lines, continue doing so
                  correctLineReceived = true;
                  requestLine = true;
                  break;
                } else {
                  correctLineReceived = true;
                  // Try to follow another master
                  requestLine = false;
                  failedLineRequests = MAX_FAILED_LINE_REQUESTS + 1; // Force a de-sync in order to elect another master
                  break;
                }
              } else {
                correctLineReceived = false;
                break;
              }
            }
          }
        }
      }

      // After firstline has been received but no other line has come along, force a line request
      if (nextLine > 0 && !correctLineReceived) {
        // Request a line with some probability
        if (random(0, 100) < 33) {
          requestLine = true;
        } else requestLine = false;
        correctLineReceived = false;
      }

      // Introduce some delay in order to de-sync line requests with other motes
      if (failedLineRequests >= MAX_FAILED_LINE_REQUESTS) {
        delayClockCycles(random(10000L, 20000L));
        failedLineRequests = 0;
      }
    }

    correctLineReceived = false;

    dataLine += 4;
    dataLineLength -= 4;

    // Is the line received OK?
    if (checkCRC(dataLine, dataLineLength)) {
      if (TYPE_OF_RECORD(dataLine) == RECTYPE_DATA) {
        // Get target address
        uint16_t addrFromHexFile = getTargetAddress(dataLine);

        // Only for the first line received
        if (firstLine) {
          firstLine = false;
          // Is the starting address from the hex file different than our user flash address?
          if (addrFromHexFile != address) {
            // Jump to user code
            jumpToUserCode();
          } else {
            // Starting address is OK
            LED_ON();
            // Erase user flash
            do {
              flash.eraseSegment((uint8_t *) address);
              address += 512;
            } while (address < USER_END_ROMADDR);

            LED_OFF();
          }
        }

        // Save vector table in buffer
        if (addrFromHexFile >= VECTOR_TABLE_ADDR) {
          uint8_t row = (addrFromHexFile - VECTOR_TABLE_ADDR);
          row /= 0x10;

          for (i = 0; i < 16; i++) {
            if (i < dataLineLength - 3)
              isrTable[row][i] = dataLine[i + 3];
            else
              isrTable[row][i] = 0xFF;
          }
        } else {
          LED_ON();
          flash.write((uint8_t *) addrFromHexFile, dataLine + 3, dataLineLength - 4);
          LED_OFF();
        }
      } else  { // Probably end of file
        // Ask for line n+1 for a while (fake line)
        lastLineNumber = receivedLineNumber;

        // Replace their reset vector with our bootloader address
        // this allows the user to provide their own interrupt vectors
        // however, the gdb boot code still runs first
#ifdef GDB_SERIAL_BOOT
        isrTable[7][0x0E] = 0x00;   // Serial bootloader address = 0x1000
        isrTable[7][0x0F] = 0x10;

        isrTable[3][0x0E] = 0x00;   // Wireless bootloader address = 0x8000
        isrTable[3][0x0F] = 0x80;
#endif

        isrTable[3][0x0C] = 0x00;   // User code address = 0xA000
        isrTable[3][0x0D] = 0xA0;
      }

      // Mark line as flashed
      markLineAsFlashed(receivedLineNumber);
    }

    // Check if it's time to execute user code (flashing done)
    if (nextNeededLineNumber() >= (lastLineNumber + 1)) {
      // Erase the vector table segment
      flash.eraseSegment((uint8_t *) VECTOR_TABLE_SEGMENT);
      // Write ISR table
      for (i = 0; i < 8; i++) {
        flash.write((uint8_t *) (VECTOR_TABLE_ADDR + i * 0x10), isrTable[i], sizeof(isrTable[i]));
      }
      // Ask for line n+1 for a while (fake line)
      fwVersionAndLineNumber = (((uint32_t)firmwareVersion) << 16) | lastLineNumber + 1;
      for (i = 0; i < 10; i++) {
        timer.start(RESPONSE_TIMEOUT);
        LED_ON();
        TRANSMIT_GWAP_QUERY_LINE(fwVersionAndLineNumber);
        LED_OFF();
        // Wait timer timeout before asking again
        while(!timer.timeout());
      }

      jumpToUserCode();
    }
  }
}


ALWAYS_INLINE
void initCore(void) {
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
}

// Return @true if the line has already been flashed
ALWAYS_INLINE
bool hasLineBeenFlashed(uint16_t lineNumber) {
  uint16_t index = lineNumber / 8;
  uint8_t mask = 1 << (lineNumber % 8);
  return (receivedLines[index] & mask) != 0;
}

// Mark a line as already flashed
ALWAYS_INLINE
void markLineAsFlashed(uint16_t lineNumber) {
  uint16_t index = lineNumber / 8;
  uint8_t mask = 1 << (lineNumber % 8);
  receivedLines[index] |= mask;
}

// Return the first not-already-written line number
ALWAYS_INLINE
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

ALWAYS_INLINE
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

ALWAYS_INLINE
uint16_t getLineNumber(uint8_t *data) {
  uint16_t lineNb = data[2];
  lineNb <<= 8;
  lineNb |= data[3];

  return lineNb;
}

ALWAYS_INLINE
uint16_t getTargetAddress(uint8_t *line) {

  uint16_t address = line[0];
  address <<= 8;
  address |= line[1];

  return address;
}

ALWAYS_INLINE
uint16_t getFwVersion(uint8_t *line) {
  uint16_t version = ((uint16_t)line[GWAP_DATA_HEAD_LEN] << 8) | line[GWAP_DATA_HEAD_LEN + 1];

  return version;
}

ALWAYS_INLINE
bool checkCRC(uint8_t *data, uint8_t len) {
  uint8_t crc = len - 4;
  uint8_t i, dataLen = len - 1;

  for (i = 0; i < dataLen; i++)
    crc += data[i];

  crc = ~crc + 1;

  if (crc == data[dataLen])
    return true;

  return false;
}

ALWAYS_INLINE
void sleep(void) {
  // Power down radio
  gwap.radio.setPowerDownState();

  // Stop WDT
  WDTCTL = WDTPW | WDTHOLD;

  // Turn off SVSH, SVSM
  PMMCTL0_H = 0xA5;
  SVSMHCTL = 0;
  SVSMLCTL = 0;
  PMMCTL0_H = 0x00;

  // Enter LPM4 with interrupts
  __bis_SR_register(LPM4_bits + GIE);
}

ALWAYS_INLINE
void jumpToUserCode(void) {
  // Exit upgrade mode
  uint8_t state = (uint8_t) SYSTATE_RESTART;
  TRANSMIT_GWAP_STATUS_STATE(state);

  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMSWBOR;
  PMMCTL0_H = 0x00;

  while(1);

//  void (*p)(void);                     // Declare a local function pointer
//  p = (void (*)(void)) USER_ROMADDR;    // Assign the pointer address
//  (*p)();                               // Call the function
}

ALWAYS_INLINE
void delayClockCycles(register uint32_t n) {
  __asm__ __volatile__ (
  "1: \n"
  " dec        %[n] \n"
  " jne        1b \n"
  :[n] "+r"(n));
}

ALWAYS_INLINE
void ledBlink(uint8_t times) {
  register uint32_t cycles = 200000000L;
  uint8_t delayTimes = 2;

  uint8_t i, j;
  for (i = 0; i < times; i++) {
    LED_ON();
    for (j = 0; j < delayTimes; j++) delayClockCycles(cycles);
    LED_OFF();
    for (j = 0; j < delayTimes; j++) delayClockCycles(cycles);
  }
}
