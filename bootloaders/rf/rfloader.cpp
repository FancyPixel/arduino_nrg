#include "rfloader.h"
#include "functions.h"

//#define WORKING_MODE MODE_4800
#define WORKING_MODE MODE_38400

extern GWAP gwap;
// Global packet
extern CCPACKET packet;
extern bool isVirgin;
extern uint8_t receivedLines[MAX_SKETCH_LINES / 8]; // We can support max (8 * MAX_SKETCH_LINES) lines of code for the sketch


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

  // This flag will tell us whether wireless bootloader needs to start or not
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
  uint16_t userRomStartingAddress = USER_CODE_STARTING_ADDR;

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
          // Is the starting address from the hex file equal to our user flash starting address?
          if (addrFromHexFile != userRomStartingAddress) {
            // Jump to user code
            jumpToUserCode();
          } else {
            // Starting address is OK
            LED_ON();
            // Erase user flash
            while (userRomStartingAddress < USER_CODE_LAST_SEGMENT_ADDR) {
              flash.eraseSegment((uint8_t *) userRomStartingAddress);
              userRomStartingAddress += FLASH_SEGMENT_SIZE;
            }

            LED_OFF();
          }
        }

        // Save vector table in buffer
        if (addrFromHexFile >= VECTOR_TABLE_ADDR) {
          uint8_t row = (addrFromHexFile - VECTOR_TABLE_ADDR);
          row /= 0x10;

//          for (i = 0; i < 16; i++) {
//            isrTable[row][i] = dataLine[i];
//          }
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

//        isrTable[3][0x0C] = 0x00;   // User code address = 0x9F80
//        isrTable[3][0x0D] = 0xA0;
        isrTable[3][0x0C] = USER_CODE_STARTING_ADDR & 0xFF;
        isrTable[3][0x0D] = (USER_CODE_STARTING_ADDR >> 8) & 0xFF;
      }

      // Mark line as flashed
      markLineAsFlashed(receivedLineNumber);
    }

    // Check if it's time to execute user code (flashing done)
    if (nextNeededLineNumber() >= (lastLineNumber + 1)) {
      // Erase the vector table segment
      // A memory segment has a size of 512 bytes
      flash.eraseSegment((uint8_t *) VECTOR_TABLE_SEGMENT);
      // Write ISR table
      for (i = 0; i < 8; i++) {
        flash.write((uint8_t *) (VECTOR_TABLE_ADDR + i * 0x10), isrTable[i], sizeof(isrTable[i]));
      }
      // Ask for line n+1 for a while (fake line)
      fwVersionAndLineNumber = (((uint32_t)firmwareVersion) << 16) | (lastLineNumber + 1);
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
