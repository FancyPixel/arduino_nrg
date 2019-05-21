#ifndef _RFLOADER_H
#define _RFLOADER_H

#include <string.h>
#include "cc430f5137.h"
#include "pmm.h"
#include "cc430flash.h"
#include "memconfig.h"
#include "timer1a0.h"
#include "gwap.h"
//#include "utils.h"

#define NVOLAT_FACTORY_RESET_DONE 0x19FF
#define NO_NETWORK_SYNCWORD_1 0xFF
#define NO_NETWORK_SYNCWORD_0 0xFF
#define DEFAULT_ACCEL_SENSIBILITY 250  // The lower the number, the higher the sensibility
#define NVOLAT_WORKING_NETWORKID_ADDR 0x20  //  2 bytes reg // Matches NVOLAT_FIRST_CUSTOM defined in gwap/nvolat.h
#define NVOLAT_WORKING_AES_PASSWORD 0x22    // 16 bytes reg
#define NVOLAT_PACKET_KEY 0x32              //  4 bytes reg
#define NVOLAT_BUMP_COUNT 0x36              //  4 bytes reg
#define NVOLAT_OFFSET_DEGREES 0x3A          //  2 bytes reg
#define NVOLAT_ACCEL_SENSIBILITY 0x3C       //  1 byte reg

/**
 * Uncomment only in case of using GDB bootloader
 */
//#define GDB_SERIAL_BOOT

/**
 * Default product code
 */
//uint8_t DEFAULT_PRODUCT_CODE[] = {0,0,0,1,0xFF,0,0,0};

/**
 * GWAP status packets
 */
// Transmit GWAP product code
#define TRANSMIT_GWAP_STATUS_PCODE()     gwap.sendPacket((uint8_t)GWAPFUNCT_STA, (uint8_t)REGI_PRODUCTCODE, (uint8_t*)GWAP_PRODUCT_CODE, sizeof(GWAP_PRODUCT_CODE))
// Transmit GWAP state
#define TRANSMIT_GWAP_STATUS_STATE(state)     gwap.sendPacketVal((uint8_t)GWAPFUNCT_STA, (uint8_t)REGI_SYSSTATE, state)
// Query product code from node with address 1
#define TRANSMIT_GWAP_QUERY_PCODE()           gwap.sendPacket((uint8_t)GWAPFUNCT_QRY, (uint8_t)REGI_PRODUCTCODE, 0, 0)
// Query firmware line from node with address 1
#define TRANSMIT_GWAP_QUERY_LINE(line)        gwap.sendPacketVal((uint8_t)GWAPFUNCT_QRY, (uint8_t)REGI_FWVERSION, line)
// Type of record
#define TYPE_OF_RECORD(line)                  line[2]

/**
 * LED operations
 */
// #define CONFIG_LED()  PJDIR |= BIT1
// #define LED_ON()      PJOUT |= BIT1
// #define LED_OFF()     PJOUT &= ~BIT1

// /**
//  * NRG2 LED operations
//  */
#define CONFIG_LED()  P3DIR |= BIT7; P3OUT &= ~BIT7
#define LED_ON()      P3OUT |= BIT7
#define LED_OFF()     P3OUT &= ~BIT7

#define CONFIG_MORSE_OUT()  P1DIR |= BIT7; P1OUT &= ~BIT7
#define MORSE_OUT_ON()      P1OUT |= BIT7
#define MORSE_OUT_OFF()     P1OUT &= ~BIT7

#define CONFIG_RESET_PIN()  P2DIR &= ~BIT7
#define IS_RESET_PIN_LOW()  (P2IN & BIT7) == 0

/**
 * Type of HEX file record
 */
#define RECTYPE_DATA  0x00
#define RECTYPE_EOF   0x01

/**
 * Firmware server
 */
// Product code
#define FIRMSERVER_PRODUCT_CODE  {0,0,0,1,0,0,0,16}
// Maximum number of bytes per line
#define BYTES_PER_LINE  48

/**
 * GWAP object
 */
GWAP gwap;

/**
 * initCore
 *
 * Initialize CC430 core
 */
ALWAYS_INLINE
void initCore(void);

/**
 * getLineNumber
 *
 * Get number of line from payload
 *
 * @param data payload from packet received
 *
 * @return line number
 */
ALWAYS_INLINE
uint16_t getLineNumber(uint8_t *data);

/**
 * getTargetAddress
 *
 * Get address from HEX line where the write has to be done
 *
 * @param line line from HEX file
 *
 * @return target address
 */
ALWAYS_INLINE
uint16_t getTargetAddress(uint8_t *line);

/**
 * checkCRC
 *
 * Check CRC from hex line
 *
 * @param data payload from packet received
 * @param len packet length
 *
 * @return true if CRC is correct
 */
ALWAYS_INLINE
bool checkCRC(uint8_t *data, uint8_t len);

/**
 * sleep
 *
 * Enter low-power mode
 */
ALWAYS_INLINE
void sleep(void);

/**
 * jumpToUserCode
 *
 * Jump to user code after exiting GWAP upgrade mode
 */
ALWAYS_INLINE
void jumpToUserCode(void);



/**
 * delayClockCycles
 *
 * Clock cycle delay
 *
 * @param n clock cycles to wait
 */
// ALWAYS_INLINE
void delayClockCycles(register uint32_t n);


#endif
