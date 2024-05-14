//
// Created by Alessandro Verlato on 16/11/22.
//

#ifndef RF_BOOTLOADER_FUNCTIONS_H
#define RF_BOOTLOADER_FUNCTIONS_H

#include <stdlib.h>
#include <string.h>
#include "cc430f5137.h"
#include "pmm.h"
#include "cc430flash.h"
#include "memconfig.h"
#include "timer1a0.h"
#include "gwap.h"

/**
 * Uncomment only in case of using GDB bootloader
 */
//#define GDB_SERIAL_BOOT

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// Transmit GWAP state
#define TRANSMIT_GWAP_STATUS_STATE(state)     gwap.sendPacketVal((uint8_t)GWAPFUNCT_STA, (uint8_t)REGI_SYSSTATE, state)
// Type of record
#define TYPE_OF_RECORD(line)                  line[2]

#define CONFIG_LED()  P3DIR |= BIT7; P3OUT &= ~BIT7
#define LED_ON()      P3OUT |= BIT7
#define LED_OFF()     P3OUT &= ~BIT7

#define CONFIG_MORSE_OUT()  P1DIR |= BIT7; P1OUT &= ~BIT7
#define MORSE_OUT_ON()      P1OUT |= BIT7
#define MORSE_OUT_OFF()     P1OUT &= ~BIT7

#define CONFIG_RESET_PIN()  P2DIR &= ~BIT7
#define IS_RESET_PIN_LOW()  (P2IN & BIT7) == 0

#define RECTYPE_DATA  0x00
#define RECTYPE_EOF   0x01

// Max payload size
#define MAX_PAYLOAD_BYTES  45
// Number of bytes for representing Firmware version
#define FWVERSION_BYTES_COUNT 2
// Number of bytes for representing Line number
#define LINE_NUMBER_BYTES_COUNT 2
// Number of bytes for representing First firmware line length
#define FIRST_FW_LINE_LEN_BYTES_COUNT 1
// Number of bytes for representing Firmware line length
#define FW_LINE_LEN_BYTES_COUNT 20
// Number of bytes for representing CRC
#define CRC_BYTES_COUNT 1

#define GWAP_QUERY_BYTES_COUNT 6

// Responses from server have to be received before 200 ms after sending the query
#define RESPONSE_TIMEOUT 500
#define MAX_FAILED_LINE_REQUESTS 3

#define MAX_SKETCH_LINES 1600

// Capabilities
#define CAPABILITY_2LINES 0x01

uint8_t calculateCrc(uint8_t *data, uint8_t len);
bool checkCRC(uint8_t *data, uint8_t len);
bool checkForFactoryReset();
uint8_t* createQueryDataFrom(uint8_t *buf, uint16_t firmwareVersion, uint16_t lineNumber, uint16_t capabilities);
void delayClockCycles(register uint32_t n);
void directJump();
void eraseUROM(bool eraseINFO = false);
void factoryReset();
uint16_t getCapabilities();
uint16_t getFwVersion(uint8_t *line);
uint16_t getLineNumber(uint8_t *data);
uint16_t getTargetAddress(uint8_t *line);
bool hasLineBeenFlashed(uint16_t lineNumber);
void initCore(void);
void jumpToUserCode(void);
void markLineAsFlashed(uint16_t lineNumber);
uint16_t nextNeededLineNumber();
uint32_t random(uint32_t min_num, uint32_t max_num);
bool readHexLine();
bool transmitGwapQueryLine(uint8_t *data);
void triggerBOR();

#endif //RF_BOOTLOADER_FUNCTIONS_H
