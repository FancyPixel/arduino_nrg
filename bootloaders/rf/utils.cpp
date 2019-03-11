#include "utils.h"

void delayClockCycles(register uint32_t n)
{
    __asm__ __volatile__ (
                "1: \n"
                " dec        %[n] \n"
                " jne        1b \n"
        : [n] "+r"(n));
}

void delay(int millis) {
  for (int i = 0; i < millis; i++) {
    delayClockCycles(200L);
  }
}

void blink(int times, int time = 500) {
  for (int i = 0; i < times; i++) {
    LED_ON();
    delay(time);
    LED_OFF();
    if (i < times -1) {
      delay(time);
    }
  }
}

int stringLen(char* string) {
  int i = 0;
  while (string[i] != '\0') {
    i++;
  }
  
  return i;
}

// Morse code stuff

char* char2morse(char c) {
  switch (c) {
    case 'a':
      return (char*)".-";

    case 'b':
      return (char*)"-...";

    case 'c':
      return (char*)"-.-.";

    case 'd':
      return (char*)"-..";

    case 'e':
      return (char*)".";

    case 'f':
      return (char*)"..-.";

    case 'g':
      return (char*)"--.";

    case 'h':
      return (char*)"....";

    case 'i':
      return (char*)"..";

    case 'j':
      return (char*)".---";

    case 'k':
      return (char*)"-.-";

    case 'l':
      return (char*)".-..";

    case 'm':
      return (char*)"--";

    case 'n':
      return (char*)"-.";

    case 'o':
      return (char*)"---";

    case 'p':
      return (char*)".--.";

    case 'q':
      return (char*)"--.-";

    case 'r':
      return (char*)".-.";

    case 's':
      return (char*)"...";

    case 't':
      return (char*)"-";

    case 'u':
      return (char*)"..-";

    case 'v':
      return (char*)"...-";

    case 'w':
      return (char*)".--";

    case 'x':
      return (char*)"-..-";

    case 'y':
      return (char*)"-.--";

    case 'z':
      return (char*)"--..";

    case '0':
      return (char*)"-----";

    case '1':
      return (char*)".----";

    case '2':
      return (char*)"..---";

    case '3':
      return (char*)"...--";

    case '4':
      return (char*)"....-";

    case '5':
      return (char*)".....";

    case '6':
      return (char*)"-....";

    case '7':
      return (char*)"--...";

    case '8':
      return (char*)"---..";

    case '9':
      return (char*)"----.";

    case ' ':
      return (char*)" ";

    default:
      return (char*)" ";
  }
}

void pullHigh() {
  LED_OFF(); // Inverse logic in order to see the led blink
  MORSE_OUT_ON();
}

void pullLow() {
  LED_ON();  // Inverse logic in order to see the led blink
  MORSE_OUT_OFF();
}

void startCommunication() {
  pullLow();
  delay(DOT_DURATION);
  pullHigh();
  delay(DOT_DURATION);

}

void endCommunication() {
  pullLow();
  delay(15 * DOT_DURATION);
  pullHigh();
  delay(DOT_DURATION);
}

// Inverse logic
void morseBlink(int duration) {
  pullLow();
  delay(duration);
  pullHigh();
}

void flashMorseChar(char c) {
  char* morseChar = char2morse(c);
  int morseCodeSize = stringLen(morseChar);
  for(int i = 0; i < morseCodeSize; i++) {
    if (morseChar[i] == '.') {
      morseBlink(DOT_DURATION);
    } else if (morseChar[i] == '-') {
      morseBlink(3 * DOT_DURATION);
    }
    // If this isn't the last component of the code, add one unit "spacer"
    if (i < (morseCodeSize - 1)) {
      delay(DOT_DURATION);
    }
  }
}


void flashMorseString(char* text) {
  int size = stringLen(text);

  startCommunication();

  for(int i = 0; i < size; i++) {
    if (text[i] == ' ') {
      // Wait 7 * DOT_DURATION (morse space between words)
      delay(7 * DOT_DURATION);
    } else {
      flashMorseChar(text[i]);
      delay(3 * DOT_DURATION);
    }
  }

  endCommunication();
}

void flashMorseString(int n) {
  char buffer[50];
  int i = 0;

  bool isNeg = n < 0;

  unsigned int n1 = isNeg ? -n : n;

  while (n1 != 0) {
    buffer[i++] = n1 % 10 + '0';
    n1 = n1 / 10;
  }

  if (isNeg)
    buffer[i++] = '-';

  buffer[i] = '\0';

  for (int t = 0; t < i / 2; t++) {
    buffer[t] ^= buffer[i - t - 1];
    buffer[i - t - 1] ^= buffer[t];
    buffer[t] ^= buffer[i - t - 1];
  }

  if (n == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
  }

  flashMorseString(buffer);
}
