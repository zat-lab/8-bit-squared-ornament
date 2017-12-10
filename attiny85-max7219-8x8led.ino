#include <MaxMatrix.h>



// bytes are in columns
// bits are bottom to top in each byte
// for less and 8 tall, shift right (ie design sprite at top)
const unsigned char SPRITES[] PROGMEM = {
  5, 8, B11111010, B10011101, B11111110, B10011101, B11111010, B00000000, B00000000, B00000000, // 0, present
  6, 7, B00011000, B00111100, B01111011, B01111011, B00111100, B00011000, B00000000, B00000000, // 1, ornament
  5, 7, B00000110, B00001111, B00000011, B01111111, B01111110, B00000000, B00000000, B00000000, // 2, candy cane
  7, 7, B00100000, B01011000, B01011100, B01011110, B01011111, B01011010, B00100100, B00000000, // 3, santa hat
  7, 7, B00100000, B01110000, B01110010, B01111101, B01111101, B00111101, B00000010, B00000000, // 4, stocking
  8, 8, B00001111, B00000100, B00010011, B11000000, B11000000, B00010011, B00000100, B00001111, // 5, rudolph
  7, 7, B00100000, B00011000, B00110110, B01111111, B00110110, B00011000, B00100000, B00000000, // 6, tree
  7, 8, B01100000, B11110110, B11111111, B11111111, B11110110, B01100000, B00000000, B00000000, // 7, snow man
  8, 7, B01001111, B01011110, B01111000, B01011000, B01011010, B01111110, B01001100, B00100000, // 8, sleigh
  7, 7, B01010101, B00100010, B00011100, B01111111, B00011100, B00100010, B01010101, B00000000, // 9, snowflake
  8, 8, B10000110, B11001001, B01101001, B00111110, B00111110, B01101001, B11001001, B10000110, // 10, ribbon
  5, 8, B10000000, B11111010, B11111101, B11111010, B10000000, B00000000, B00000000, B00000000, // 11, candle
};
#define ROW_SIZE 10 /* 1 width byte, 1 bit depth byte, 8 data bytes */
// #define MAX_SPRITES 8 /* max index, not total number */
const int MAX_SPRITES = sizeof(SPRITES) / ROW_SIZE - 1;

#define DIN 0 /* physical pin 5 */
#define CS 1 /* physical pin 6 */
#define CLK 2 /* physical pin 7 */

#define MAXS 1 /* number of displays */

MaxMatrix m(DIN, CS, CLK, MAXS); // define Library

byte buff[ROW_SIZE]; // temp space between "progmem" and the library

unsigned long nextBitmapChange = 0;
#define BITMAP_CHANGE_INT 1250 /* is ATTINY85 timing off? */

byte c = 0;

unsigned long t;

// #define BUTTON_PIN 4 /* physical pin 3 */ not working?
#define BUTTON_PIN 3 /* physical pin 2 */ 
int buttonChanged;

#define BUTTON_DEBOUNCE_DELAY 75


void setup() {
  m.init();
  m.setIntensity(1);  // LED Intensity 0-15, was 2
  m.clear();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}


void printBitmap(byte x, byte y, const byte* b) {
  // m.writeSprite(int x, int y, const byte* sprite)
  m.writeSprite(x, y, b);
}


void printBitmapRandomLoc(byte c, byte erase) {
  if (erase == 1) m.clear();

  // print requested character
  memcpy_P(buff, SPRITES + ROW_SIZE * c, ROW_SIZE);
  // random (min inclusive, max exclusive)
  printBitmap(random(0, 9 - buff[0]), random(0, 9 - buff[1]), buff);
};


int debouceButton(int pin) {
  static unsigned long nextButtonDebounceTime;
  static int lastButtonValue = HIGH; // pin is in a pullup state
  static int lastReadValue = HIGH; // ditto
  
  int b = digitalRead(pin);
  
  // t = millis();

  if (b != lastReadValue) {
    nextButtonDebounceTime = t + BUTTON_DEBOUNCE_DELAY;
    lastReadValue = b;
  }

  if (t > nextButtonDebounceTime) {
    lastButtonValue = b;
    return b;
  } else {
    return lastButtonValue;
  }
}


int buttonChangedToLow() {
  static int lastButtonValue = HIGH; // pin is in a pullup state
  int buttonValue = debouceButton(BUTTON_PIN);

  if ((lastButtonValue == HIGH) && (buttonValue == LOW)) {
    lastButtonValue = LOW;
    return 1;
  } else {
    lastButtonValue = buttonValue;
    return 0;
  }
}


void loop() {
  t = millis();
  
  buttonChanged = buttonChangedToLow();
  
  if (buttonChanged == 1) {
    c++;
    if (c > MAX_SPRITES) c = 0; // not 0 as that's "blank"
  }
  
  
  if ((t > nextBitmapChange) || (buttonChanged == 1)) {
    nextBitmapChange = t + BITMAP_CHANGE_INT;
    printBitmapRandomLoc(c, 1);
  }
  
  delay(1);
}
