#ifndef _UTILS_H
#define _UTILS_H


#include "cc430f5137.h"
#include "pmm.h"
#include "cc430flash.h"
#include "memconfig.h"
#include "timer1a0.h"

#define CONFIG_LED()  P3DIR |= BIT7; P3OUT &= ~BIT7
#define LED_ON()      P3OUT |= BIT7
#define LED_OFF()     P3OUT &= ~BIT7

#define CONFIG_MORSE_OUT()  P1DIR |= BIT7; P1OUT &= ~BIT7
#define MORSE_OUT_ON()      P1OUT |= BIT7
#define MORSE_OUT_OFF()     P1OUT &= ~BIT7

#define DOT_DURATION 7

void delayClockCycles(register uint32_t n);

void delay(int millis);

void blink(int times, int time);

char* char2morse(char c);

void pullHigh();

void pullLow();

void startCommunication();
void endCommunication();
void morseBlink(int duration);
void flashMorseChar(char c);
void flashMorseString(char* text);
void flashMorseString(int i);

#endif
