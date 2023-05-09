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

// Transmit GWAP state
#define TRANSMIT_GWAP_STATUS_STATE(state)     gwap.sendPacketVal((uint8_t)GWAPFUNCT_STA, (uint8_t)REGI_SYSSTATE, state)
// Query firmware line from node with address 1
#define TRANSMIT_GWAP_QUERY_LINE(line)        gwap.sendPacketVal((uint8_t)GWAPFUNCT_QRY, (uint8_t)REGI_FWVERSION, line)
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

// Maximum number of bytes of payload:  fwVersion + lineNumber + line1data + (line2data) + crc
//                                          2            2           20      20 optional    1
#define MAX_PAYLOAD_BYTES  45
// Firmware version length
#define FWVERSION_LEN_BYTES 2
// Line number length
#define LINE_NUMBER_LEN_BYTES 2
// Firmware line length
#define FW_LINE_LEN_BYTES 20
// CRC length
#define CRC_LEN_BYTES 1

// Responses from server have to be received before 200 ms after sending the query
#define RESPONSE_TIMEOUT 500
#define MAX_SKETCH_LINES 1600
#define MAX_FAILED_LINE_REQUESTS 3


/**
 * Uncomment only in case of using GDB bootloader
 */
//#define GDB_SERIAL_BOOT


// Transmit GWAP state
#define TRANSMIT_GWAP_STATUS_STATE(state)     gwap.sendPacketVal((uint8_t)GWAPFUNCT_STA, (uint8_t)REGI_SYSSTATE, state)
// Query firmware line from node with address 1
#define TRANSMIT_GWAP_QUERY_LINE(line)        gwap.sendPacketVal((uint8_t)GWAPFUNCT_QRY, (uint8_t)REGI_FWVERSION, line)
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

void initCore(void);

bool readHexLine();

bool hasLineBeenFlashed(uint16_t lineNumber);

void markLineAsFlashed(uint16_t lineNumber);

uint16_t nextNeededLineNumber();

void factoryReset();

uint16_t getLineNumber(uint8_t *data);

uint16_t getTargetAddress(uint8_t *line);

uint16_t getFwVersion(uint8_t *line);

bool checkCRC(uint8_t *data, uint8_t len);

void sleep(void);

void jumpToUserCode(void);

void delayClockCycles(register uint32_t n);

uint32_t random(uint32_t min_num, uint32_t max_num);

void ledBlink(uint8_t times);

#endif //RF_BOOTLOADER_FUNCTIONS_H
