//#include "HardwareSerial.h"
#include "rfloader.h"
#include "functions.h"
#include "utils.h"

//#define DEBUG true

#define WORKING_MODE MODE_38400

extern GWAP gwap;
// Global packet
extern CCPACKET packet;
extern CC430FLASH flash;
//extern bool justFactoryReset;
extern uint8_t receivedLines[MAX_SKETCH_LINES / 8]; // We can support max (8 * MAX_SKETCH_LINES) lines of code for the sketch

int main(void) {
  bool firstLine = true;
  // Length of last line received
  uint8_t dataLineLength = 0, currentLineLength = 0;
  // Pointer to line buffer
  uint8_t *dataLine;
  uint8_t *queryData = (uint8_t*)malloc(sizeof(uint8_t) * GWAP_QUERY_BYTES_COUNT);
  // User code address
  uint16_t userCodeAddr;
  // Bootloader version high word
  uint16_t bootloaderVersionH;
  // Bootloader version low word
  uint16_t bootloaderVersionL;
  bool correctLineReceived = false;
  uint16_t firmwareVersion = 0xFFFF;
  bool requestLine = true, parsingFirstLine = true, finishedParsing = false;
  uint16_t _fwVersion = 0xFFFF; // This must be here
  uint16_t failedLineRequests = 0;
  uint16_t receivedLineNumber = 0;
  uint16_t neededLineNum, fwLastLineNumber = 0xFFFE;
  bool justFactoryReset = false;
  // ISR vector table
  uint8_t isrTable[8][16];

  /*
   * *** IMPORTANT ***
   * DO NOT CHANGE the order of the code lines if you want to have a working morse decoding
   */
  CONFIG_LED();
  #ifdef DEBUG
    CONFIG_MORSE_OUT();
  #endif
  CONFIG_RESET_PIN();

  // *** You must uncomment this line in order to startup morse decoding ***
  #ifdef DEBUG
    flashMorseString("start\n");
  #endif

  // Some fancy blinking for signaling that we're on bootloader
  for (uint8_t j = 0; j < 5; j++) {
    LED_ON();
    delayClockCycles(5000);
    LED_OFF();
    delayClockCycles(5000);
  }
  // This flag will tell us whether wireless bootloader needs to start or not
  bool *ptr1;
  ptr1 = (bool*) RAM_END_ADDRESS;   // Memory address at the end of the stack
  bool runUserCode = *ptr1;           // Read value. If "false" it means we're coming from sketch space

  // Read user code starting address
  uint16_t *ptr2;
  ptr2 = (uint16_t*) USER_RESET_VECTOR;
  userCodeAddr = *ptr2;

  //Read and update bootloader version
  uint16_t *ptr3;
  ptr3 = (uint16_t*) BL_VERSION_H_VECTOR;
  bootloaderVersionH = *ptr3;
  
  uint16_t *ptr4;
  ptr4 = (uint16_t*) BL_VERSION_L_VECTOR;
  bootloaderVersionL = *ptr4;

  uint16_t updatedBootloaderVersionH = (FIRMWARE_VERSION[0] << 8) | FIRMWARE_VERSION[1];
  uint16_t updatedBootloaderVersionL = (FIRMWARE_VERSION[2] << 8) | FIRMWARE_VERSION[3];

  if (bootloaderVersionH != updatedBootloaderVersionH || bootloaderVersionL != updatedBootloaderVersionL ) { 
    flash.update((unsigned char*)FIRMWARE_VERSION, 0xFE00, 0x1B6, sizeof(FIRMWARE_VERSION));
  }

   // Disable interrupts
  __disable_interrupt();

  // Init core
  initCore();


  //  Check for factory reset
  if (checkForFactoryReset()) {
    factoryReset();
    justFactoryReset = true;
  } else {
    // Check if firmware exists
    if (userCodeAddr != 0xFFFF) {
      if (runUserCode == false) {
       // chiedi righe
      } else {
        jumpToUserCode();
      }
    }
  } 


  TIMER1A0 timer;

  // Init GWAP comms
  gwap.init(CFREQ_868, WORKING_MODE);

  // Pointer at the begining of user flash
  uint16_t userRomStartingAddress = USER_CODE_STARTING_ADDR;

  while (1) {
    while (!correctLineReceived) {
      neededLineNum = nextNeededLineNumber();
      if (requestLine) {
        LED_ON();
        // Combine fwVersion, needed line number and capabilities and query firmware line
        createQueryDataFrom(queryData, firmwareVersion, neededLineNum, getCapabilities());
        transmitGwapQueryLine(queryData);
        LED_OFF();
        failedLineRequests++;
      }

      // Start timer
      timer.start(RESPONSE_TIMEOUT);
      while (!timer.timeout()) {
        uint8_t status = ReadSingleReg(PKTSTATUS);
        uint8_t bytes = ReadSingleReg(RXBYTES);

        // Poll PKSTATUS and number of bytes in the Rx FIFO
        if ((status & 0x01) && bytes) {
          while (ReadSingleReg(PKTSTATUS) & 0x01);

          // Max CC1101 packet length:  62 bytes
          // Received packet example
          // Data         // 2f000a0072e1694700000004   00   00   02   0064    0000          14        94000055425c0135d0085a8245ea1f3140fe2b97     (9410003f4076000f9308249242ea1f5c012f839d)    ae     //  raw segments
          // Meaning      //       moteUid              cn   fn   reg  fwVer  line num   line N len                 line N data                                (line N+1 data)                    crc    //  segment meaning
          // Bytes count  //          12                1    1    1     2        2           1                         20                                           (20)                          1      //  segment length (bytes)

          // Single-line total packet length: 41 bytes
          // Two-lines total packet length: 61 bytes

          // 2f000a0072e1694700000004 00 00 02 0064 0000 14  94000055425c0135d0085a8245ea1f3140fe2b97  9410003f4076000f9308249242ea1f5c012f839d  9a

          // Packet received.

          // Check if it's time to execute user code (flashing done)
          // We must jump to user code if we already received the last line, and the next needed line number is greater than last firmware line

          if (neededLineNum >= fwLastLineNumber) {
            // Erase the vector table segment
            // A memory segment has a size of 512 bytes
            flash.eraseSegment((uint8_t *) VECTOR_TABLE_SEGMENT);

            // Replace their reset vector with our bootloader address
            // this allows the user to provide their own interrupt vectors
            // however, the gdb boot code still runs first
            #ifdef GDB_SERIAL_BOOT
              isrTable[7][0x0E] = 0x00;   // Serial bootloader address = 0x1000
              isrTable[7][0x0F] = 0x10;

              isrTable[3][0x0E] = 0x00;   // Wireless bootloader address = 0x8000
              isrTable[3][0x0F] = 0x80;
            #endif

            //      FFFFFFFFFFFFFFFFFFFFFFFF0096FFFF
            // FFB0 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
            // FFB  0 1 2 3 4 5 6 7 8 9 A B C D E F
            //      FFFFFFFFFFFFFFFFFFFFFFFF0096FFFF
            isrTable[3][0x06] = FIRMWARE_VERSION[1];
            isrTable[3][0x07] = FIRMWARE_VERSION[0];
            isrTable[3][0x08] = FIRMWARE_VERSION[3];
            isrTable[3][0x09] = FIRMWARE_VERSION[2];

            // TODO: Cambiare le due righe qui sotto
            //       Non ci si può più basare su USER_CODE_STARTING_ADDR, ma bisogna andare a leggere
            //       isrTable[7][0x0E] e isrTable[7][0x0F]

//            isrTable[3][0x0C] = USER_CODE_STARTING_ADDR & 0xFF;
//            isrTable[3][0x0D] = (USER_CODE_STARTING_ADDR >> 8) & 0xFF;
            isrTable[3][0x0C] = isrTable[7][0x0E];
            isrTable[3][0x0D] = isrTable[7][0x0F];
            isrTable[7][0x0E] = BOOTLOADER_STARTING_ADDR & 0xFF;
            isrTable[7][0x0F] = (BOOTLOADER_STARTING_ADDR >> 8) & 0xFF;

            // Write ISR table
            for (uint8_t i = 0; i < sizeof(isrTable)/sizeof(isrTable[0]); i++) {
              flash.write((uint8_t *)VECTOR_TABLE_ADDR + (i * sizeof(isrTable[i])), isrTable[i], sizeof(isrTable[i]));
            }

            jumpToUserCode();
          }

          // Read packet and extract HEX line
          if (readHexLine()) {  // This also checks PRODUCT_CODE
            if (!checkCRC(packet.data, packet.length)) {
              correctLineReceived = false;
              requestLine = true;
              break;
            }
            failedLineRequests = 0;

            // *** RF Packet OK: crc ok, function = status, PRODUCT_CODE ok, regId = firmware (02) ***

            // Extract firmware version
            _fwVersion = getFwVersion(packet.data);
            // Data payload
            dataLine = packet.data + GWAP_DATA_HEAD_LEN + FWVERSION_BYTES_COUNT;  // Jump to byte #17 (first byte of line number)
            dataLineLength = packet.length - GWAP_DATA_HEAD_LEN - FWVERSION_BYTES_COUNT - 1;

            // Extract line number
            // When we receive 2 fw data lines, we consider the second line to have  lineNumber = receivedLineNumber + 1
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

      // After firstLine has been received but no other line has come along, force a line request
      if (neededLineNum > 0 && !correctLineReceived) {
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

    dataLine += 2;  // Jump to byte #19 (first line len)
    dataLineLength -= 2;

    // Parse received lines
    parsingFirstLine = true;
    finishedParsing = false;
    // While we still have lines to read...
    while(!finishedParsing && (dataLineLength > 0)) {
      if (parsingFirstLine) {
        currentLineLength = dataLine[0];

        dataLine += 1; // Jump to first byte of current line
        dataLineLength -= 1;

        parsingFirstLine = false;
      } else {
        // Empty current line
        currentLineLength = dataLineLength;
        finishedParsing = true;
        receivedLineNumber++;
      }

      if (TYPE_OF_RECORD(dataLine) == RECTYPE_DATA) {
        // Get target address
        uint16_t addrFromHexFile = getTargetAddress(dataLine);

        // Only for the first line received
        if (firstLine) {
          // Is the starting address from the hex file equal to our user flash starting address?
          if (addrFromHexFile != userRomStartingAddress) {
            // Jump to user code
           jumpToUserCode();
          } else {
            firstLine = false;
            // Starting address is OK
            // Erase user flash
            if (!justFactoryReset) eraseUROM();
          }
        }

        // Save vector table in buffer
        if (addrFromHexFile >= VECTOR_TABLE_ADDR) {
          uint8_t row = (addrFromHexFile - VECTOR_TABLE_ADDR);
          row /= 0x10;

          for (uint8_t i = 0; i < 0x10; i++) {
            if (i < currentLineLength - 3)
              isrTable[row][i] = dataLine[i + 3];
            else
              isrTable[row][i] = 0xFF;
          }
        } else {
          // Flash firmware line
          LED_ON();
          flash.write((uint8_t *) addrFromHexFile, dataLine + 3, currentLineLength - 4);
          LED_OFF();
        }
      } else if ((TYPE_OF_RECORD(dataLine) == RECTYPE_EOF)) { // End of file
        fwLastLineNumber = receivedLineNumber;
      }

      dataLine += currentLineLength; // Jump to first byte of next line
      dataLineLength -= currentLineLength;

      // Mark line as flashed
      markLineAsFlashed(receivedLineNumber);
    }
  }
}
