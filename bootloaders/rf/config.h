#ifndef _CONFIG_H
#define _CONFIG_H

/**
 * Repeater options
 */
// Amount of transactions to be saved for evaluation before repeating a packet
// Maximum depth = 255
#define REPEATER_TABLE_DEPTH  20
// Expiration time (in ms) for any transaction
#define REPEATER_EXPIRATION_TIME  2000

/**
 * Addressing schema
 */
// Extended addresses (2 bytes)
//#define SWAP_EXTENDED_ADDRESS     1

#endif

