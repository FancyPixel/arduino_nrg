#ifndef _DATATYPES_H
#define _DATATYPES_H

#ifdef __GNUC__
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NEVER_INLINE __attribute__((noinline))
#else
#define ALWAYS_INLINE inline
#define NEVER_INLINE
#warning This code takes advantage of features only found in msp430-gcc!!!
#endif

#define uint8_t unsigned char
#define uint16_t unsigned int
#define uint32_t unsigned long

#endif

