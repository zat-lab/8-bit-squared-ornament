#include <MaxMatrix.h>


/*
 * @TODO menu items
 * - random at start
 * - auto-advance
 * - auto-advance interval
 * - brightness
 * - walk/random interval
 * - custom animation interval
 * 
 * save settings to ee-prom
 * use long press to get to menu
 */


// Animations
#define ANI_NONE 0
#define ANI_RANDOM 1
#define ANI_WALK 2
#define ANI_CUSTOM 3

/**
 * byte 
 * 0: x-width
 * 1: y-width
 * 2-9: bitmap
 * 10: animations - see above
 * 
 */
// bytes are in columns
// bits are bottom to top in each byte
// for less and 8 tall, shift right (ie design sprite at top)
const unsigned char SPRITES[] PROGMEM = {
  5, 8, B11111010, B10011101, B11111110, B10011101, B11111010, B00000000, B00000000, B00000000, ANI_WALK, // 0, present
  6, 7, B00011000, B00111100, B01111011, B01111011, B00111100, B00011000, B00000000, B00000000, ANI_RANDOM, // 1, ornament
  5, 7, B00000110, B00001111, B00000011, B01111111, B01111110, B00000000, B00000000, B00000000, ANI_RANDOM, // 2, candy cane
  7, 7, B00100000, B01011000, B01011100, B01011110, B01011111, B01011010, B00100100, B00000000, ANI_WALK, // 3, santa hat
  7, 7, B00100000, B01110000, B01110010, B01111101, B01111101, B00111101, B00000010, B00000000, ANI_WALK, // 4, stocking
  8, 8, B00001111, B00000100, B00010011, B11000000, B11000000, B00010011, B00000100, B00001111, ANI_CUSTOM, // 5, rudolph
  7, 7, B00100000, B00011000, B00110110, B01111111, B00110110, B00011000, B00100000, B00000000, ANI_NONE, // 6, tree
  7, 8, B01100000, B11110110, B11111111, B11111111, B11110110, B01100000, B00000000, B00000000, ANI_WALK, // 7, snow man
  8, 7, B01001111, B01011110, B01111000, B01011000, B01011010, B01111110, B01001100, B00100000, ANI_WALK, // 8, sleigh
  7, 7, B01010101, B00100010, B00011100, B01111111, B00011100, B00100010, B01010101, B00000000, ANI_WALK, // 9, snowflake
  8, 8, B10000110, B11001001, B01101001, B00111110, B00111110, B01101001, B11001001, B10000110, ANI_NONE, // 10, ribbon
  5, 8, B10000000, B11111010, B11111101, B11111010, B10000000, B00000000, B00000000, B00000000, ANI_WALK, // 11, candle
  6, 6, B00010000, B00011100, B00111111, B00111111, B00011100, B00010000, B00000000, B00000000, ANI_WALK, // 12, bell
};
#define ROW_SIZE 11 /* 1 width byte, 1 bit depth byte, 8 data bytes */
const int MAX_SPRITES = sizeof(SPRITES) / ROW_SIZE - 1;



#define DIN 0 /* physical pin 5 */
#define CS 1 /* physical pin 6 */
#define CLK 2 /* physical pin 7 */

#define MAXS 1 /* number of displays */

MaxMatrix m(DIN, CS, CLK, MAXS); // define Library

#define CURRENT_SPRITE_SIZE 10
byte currentSprite[CURRENT_SPRITE_SIZE]; // temp space between "progmem" and the library

unsigned long nextBitmapChange = 0;
#define BITMAP_CHANGE_INT 1250 /* is ATTINY85 timing off? */

byte c = 0;

byte spriteX = 0;
byte spriteY = 0;

unsigned long t;

#define BUTTON_PIN 3 /* physical pin 2 */ 
int buttonChanged;

#define BUTTON_DEBOUNCE_DELAY 50


void setup() {
  m.init();
  m.setIntensity(1);  // LED Intensity 0-15, was 2
  m.clear();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  memcpy_P(currentSprite, SPRITES + ROW_SIZE * c, CURRENT_SPRITE_SIZE);
  printBitmap(spriteX, spriteY, currentSprite);
}


void printBitmap(byte x, byte y, const byte* b) {
  // m.writeSprite(int x, int y, const byte* sprite)
  m.writeSprite(x, y, b);
}


void printBitmapRandomWalk(byte c) {
  // memcpy_P(currentSprite, SPRITES + ROW_SIZE * c, ROW_SIZE); // done on demand only
  
  int tempX = spriteX + random(0, 3) - 1;
  int tempY = spriteY + random(0, 3) - 1;

  if (tempX < 0) tempX = 0;
  else if (tempX > (8 - currentSprite[0])) tempX = 8 - currentSprite[0];
  

  if (tempY < 0) tempY = 0;
  else if (tempY > (8 - currentSprite[1])) tempY = 8 - currentSprite[1];
  
  if (tempX == spriteX && tempY == spriteY) return; // no change, so reduce flicker

  spriteX = tempX;
  spriteY = tempY;
  
  // erase is assumed since something changed
  m.clear();
  // print requested character
  printBitmap(spriteX, spriteY, currentSprite);
}


void printBitmapRandomLoc(byte c, byte erase) {
  if (erase == 1) m.clear();

  // print requested character
  // memcpy_P(currentSprite, SPRITES + ROW_SIZE * c, 10); // done on demand only
  // random (min inclusive, max exclusive)
  printBitmap(random(0, 9 - currentSprite[0]), random(0, 9 - currentSprite[1]), currentSprite);
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
    memcpy_P(currentSprite, SPRITES + ROW_SIZE * c, CURRENT_SPRITE_SIZE);
  }
  
  
  if ((t > nextBitmapChange) || (buttonChanged == 1)) {
    nextBitmapChange = t + BITMAP_CHANGE_INT;
    switch ((int)currentSprite[10]) {
      case ANI_NONE:
      case ANI_RANDOM:
      case ANI_WALK:
      case ANI_CUSTOM:
      default:  // @TODO
        printBitmapRandomWalk(c); // was printBitmapRandomLoc(c, 1);
    }
    
  }
  
  delay(1);
}
