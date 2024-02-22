//
// Created by Alessandro Verlato on 18/11/22.
//

#ifndef RF_BOOTLOADER_CODESIZE_H
#define RF_BOOTLOADER_CODESIZE_H

// Define bootloader code size

//  ***  DO NOT MODIFY - THIS IS AUTOMATICALLY DEFINED AT BOOTLOADER COMPILE TIME  ***

#define BOOTLOADER_CODE_SIZE 5120


// TODO: Sembra non servire in functions.cpp:87 -> eliminare dalla fase di compilazione?
//const uint8_t isrVector_FFC0[] = { 0x9A, 0x84, 0x9A, 0x84, 0x9A, 0x84, 0x9A, 0x84, 0x9A, 0x84, 0x9A, 0x84, 0x9A, 0x84, 0x9A, 0x84 };

#endif //RF_BOOTLOADER_CODESIZE_H
