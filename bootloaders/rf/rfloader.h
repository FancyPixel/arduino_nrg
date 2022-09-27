#ifndef _RFLOADER_H
#define _RFLOADER_H

#include <stdlib.h>
#include <string.h>
#include "cc430f5137.h"
#include "pmm.h"
#include "cc430flash.h"
#include "memconfig.h"
#include "timer1a0.h"
#include "gwap.h"
//#include "utils.h"


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

// Maximum number of bytes per line
#define BYTES_PER_LINE  48

GWAP gwap;

ALWAYS_INLINE
void initCore(void);

ALWAYS_INLINE
bool readHexLine();

ALWAYS_INLINE
bool hasLineBeenFlashed(uint16_t lineNumber);

ALWAYS_INLINE
void markLineAsFlashed(uint16_t lineNumber);

ALWAYS_INLINE
uint16_t nextNeededLineNumber();

ALWAYS_INLINE
void factoryReset();

ALWAYS_INLINE
uint16_t getLineNumber(uint8_t *data);

ALWAYS_INLINE
uint16_t getTargetAddress(uint8_t *line);

ALWAYS_INLINE
uint16_t getFwVersion(uint8_t *line);

ALWAYS_INLINE
bool checkCRC(uint8_t *data, uint8_t len);

ALWAYS_INLINE
void sleep(void);

ALWAYS_INLINE
void jumpToUserCode(void);

ALWAYS_INLINE
void delayClockCycles(register uint32_t n);

ALWAYS_INLINE
uint32_t random(uint32_t min_num, uint32_t max_num) {
  return rand() % (max_num + 1 - min_num) + min_num;
}

ALWAYS_INLINE
void ledBlink(uint8_t times);

#endif
