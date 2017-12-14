#include <MaxMatrix.h>
#include "main.h"

/*
 * @TODO menu items
 * (done) random [mode]
 * (done) auto-advance
 * - auto-advance interval
 * (done) brightness
 * - walk/random interval
 * - custom animation interval
 * 
 * (no space) save settings to ee-prom
 * (done) use very long press to get to menu
 */

enum appState_e {Normal, autoAdvance, shuffleAdvance, Admin};
enum appState_e appState = Normal;

// Animations
#define ANI_NONE 0
#define ANI_RANDOM 1
#define ANI_WALK 2
#define ANI_CUSTOM 3
#define ANI_WALK_REQ 4
// @TODO ANI_RANDOM_REQ // ensure it moves every time

/**
 * byte 
 * 0: x-width
 * 1: y-width
 * 2-9: bitmap
 *    bytes are in columns
 *    bits are bottom to top in each byte
 *    for less and 8 tall, shift right (ie design sprite at top)
 * 10: animation mode - see above
 */
const unsigned char SPRITES[] PROGMEM = {
  5, 8, B11111010, B10011101, B11111110, B10011101, B11111010, B00000000, B00000000, B00000000, ANI_WALK, // 0, present
  6, 7, B00011000, B00111100, B01111011, B01111011, B00111100, B00011000, B00000000, B00000000, ANI_RANDOM, // 1, ornament
  5, 7, B00000110, B00001111, B00000011, B01111111, B01111110, B00000000, B00000000, B00000000, ANI_RANDOM, // 2, candy cane
  7, 7, B00100000, B01011000, B01011100, B01011110, B01011111, B01011010, B00100100, B00000000, ANI_WALK, // 3, santa hat
  7, 7, B00100000, B01110000, B01110010, B01111101, B01111101, B00111101, B00000010, B00000000, ANI_WALK, // 4, stocking
  8, 8, B00001111, B00000100, B00010011, B11000000, B11000000, B00010011, B00000100, B00001111, ANI_CUSTOM, // 5, rudolph
  8, 8, B01000000, B00110000, B01101100, B11111111, B11111111, B01101100, B00110000, B01000000, ANI_CUSTOM, // 6, tree v2
  6, 8, B01100000, B11110110, B11111111, B11111111, B11110110, B01100000, B00000000, B00000000, ANI_WALK_REQ, // 7, snow man
  8, 7, B01001111, B01011110, B01111000, B01011000, B01011010, B01111110, B01001100, B00100000, ANI_WALK_REQ, // 8, sleigh
  7, 7, B01010101, B00100010, B00011100, B01111111, B00011100, B00100010, B01010101, B00000000, ANI_WALK_REQ, // 9, snowflake
  8, 8, B10000110, B11001001, B01101001, B00111110, B00111110, B01101001, B11001001, B10000110, ANI_NONE, // 10, ribbon
  6, 8, B00000000, B10000000, B11111010, B11111101, B11111010, B10000000, B00000000, B00000000, ANI_CUSTOM, // 11, candle
  // 6, 6, B00010000, B00011100, B00111111, B00111111, B00011100, B00010000, B00000000, B00000000, ANI_CUSTOM, // 12, bell (non-ani)
  8, 8, B00000000, B00100000, B00111000, B01111110, B01111110, B00111000, B00100000, B00000000, ANI_CUSTOM, // 12, bell v2
  8, 8, B00011000, B01100110, B01011010, B10100101, B10100101, B01011010, B01100110, B00011000, ANI_CUSTOM, // 13, wreath
  8, 8, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, ANI_CUSTOM, // 14, snow (blank)
  8, 5, B00011111, B00000100, B00011111, B00000000, B00001110, B00010001, B00010001, B00001110, ANI_CUSTOM, // 15, HO
  8, 8, B11001100, B11101100, B01110110, B00100101, B00100101, B01110110, B11101100, B11001100, ANI_CUSTOM, // 16, gingerbread man
  // star? tried but looks like crap
  // elf?
  // dove?
  // cookie? animate bites (like gingerbread man)?
  // mitten?
  // elf? (hat & pointy ears)
  // easter egg: frosty melts after a long time?
  
};
#define ROW_SIZE 11 /* 1 width byte, 1 bit depth byte, 8 data bytes */
const int MAX_SPRITES = sizeof(SPRITES) / ROW_SIZE;

void (*animations[MAX_SPRITES]) (bool newAnimation); // animation function pointer array

int animationsInt[MAX_SPRITES];

const unsigned char BELL_SPRITES[] PROGMEM = {
  8, 8, B00000000, B00000110, B00111110, B11111100, B01111100, B01111100, B00111000, B00001000, // bell bottom right
  8, 8, B00001000, B00111000, B01111100, B01111100, B11111100, B00111110, B00000110, B00000000, // bell bottom left
};

const unsigned char BLANK[] PROGMEM = {
  8, 8, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000,
};

#define DIN 0 /* physical pin 5 */
#define CS 1 /* physical pin 6 */
#define CLK 2 /* physical pin 7 */

#define MAXS 1 /* number of displays */

MaxMatrix m(DIN, CS, CLK, MAXS); // set matrix lib

unsigned char currentSprite[ROW_SIZE]; // temp space between "progmem" and the library

// @TODO is ATTINY85 timing off? seconds don't seem right
unsigned long nextBitmapChange = 0;
#define BITMAP_CHANGE_INT 1000
unsigned long nextAnimationChange = 0;
// bool newAnimation;

#define AUTO_ADVANCE_CHANGE_INT 9000
unsigned long nextAutoAdvanceChange = 0;

unsigned char c = 0;  // @TODO this should be renamed

int spriteX = 0;
int spriteY = 0;

unsigned long t; // the "current" time in the loop

#define BUTTON_PIN 3 /* physical pin 2 */ 
int buttonChanged;

#define BUTTON_DEBOUNCE_DELAY 40 /* seems to work ok */
#define BUTTON_PRESS_SHORT_DELAY 80
#define BUTTON_PRESS_LONG_DELAY 800
#define BUTTON_PRESS_VERY_LONG_DELAY 1600

enum buttonAction_e {None, Short, Long, veryLong, Double, Hold};
enum buttonAction_e buttonState;

char intensity = 2;

// #define LED_PIN 4 /* physical pin 3 @DEBUG */ 

void setup() {
  m.init();
  m.setIntensity(intensity);  // LED Intensity 0-15, 0 dimest, 15 brightest
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // pinMode(LED_PIN, OUTPUT); // @DEBUG
  // digitalWrite(LED_PIN, LOW); // @DEBUG

  // clear animations table
  for (int i = 0; i < MAX_SPRITES; i++) {
    animations[i] = NULL;
  }

  // set up animation function pointers
  animations[5] = &animationRudolph; animationsInt[5] = 800;
  animations[6] = &animationTree; animationsInt[6] = 100;
  animations[11] = &animationCandle; animationsInt[11] = 200;
  animations[12] = &animationBell; animationsInt[12] = 700;
  animations[13] = &animationWreath; animationsInt[13] = 50;
  animations[14] = &animationSnow; animationsInt[14] = 300;
  animations[15] = &animationHo; animationsInt[15] = 400;
  animations[16] = &animationGingerbreadMan; animationsInt[16] = 900;

  t = millis();
  printNewSprite();
}


// since randX, randY and randDots feed into random() they are exclusive
// @TODO add bias control (as in towards min or max rand)? or even bias x and y? or rand value?
void randomDots(int baseX, int randX, int baseY, int randY, int baseDots, int randDots, int value) {
  int numDots = random(baseDots, randDots);

  for (int i = 0; i < numDots; i++) {
    m.setDot(random(baseX, randX), random(baseY, randY), value);
  }
}


void animationGingerbreadMan(bool newAnimation) {
  static int state; // 0 pause, 1 remove random limbs (until done), then torso, 2 wait, 3 print whole, reset state
  static char parts; // 0 head, 1 right arm, 2 right leg, 3 left leg, 4 left arm, 5 torso
  int part;

  if (newAnimation) {
    state = 0;
    parts = 0;
    memcpy_P(currentSprite, BLANK, 10); // copy in blank sprite just once per animation
    return;
  }
  
  switch (state) {
    case 0:
      // do nothing (pause)
      state = 1;
      break;
    case 1:
       // check if all random parts have been removed/checked, then do torso
      if (parts == B00011111) {
        part = 5;
      } else {
        // find part we haven't removed
        do {
          part = random(0, 5); // do not include torso (5)
        } while (((parts >> part) & 1) == 1); // check if part'th bit of parts is already set to 1
      }

      // set part bit as removed
      parts |= (1 << part);

      // actual removal of parts in next switch
      break;
    case 2:
      // do nothing (pause)
      state = 3;
      break;
    case 3:
      // reprint sprite
      memcpy_P(currentSprite, SPRITES + ROW_SIZE * c, 10);
      printBitmap(0, 0, currentSprite);
      
      // reset animation
      state = 0;
      parts = 0;
      break;
  }

  if (state != 1) return; // nothing else to be done
  
  // ie state == 1, so remove parts
  // @NOTE these parts come off randomly, not in order
  switch (part) {
    case 0: // head
      currentSprite[0] = 4;
      currentSprite[1] = 2;
      printBitmap(2, 0, currentSprite);
      
      // crumbs
      randomDots(1, 7, 0, 2, 1, 3, 1);
      break;
    case 1: // right arm
      currentSprite[0] = 3;
      currentSprite[1] = 2;
      // close up torso
      currentSprite[2 + 0] = B00000011; // row zero
      printBitmap(5, 2, currentSprite);
      // reset blank
      currentSprite[2 + 0] = 0;
      
      // crumbs
      randomDots(6, 8, 1, 5, 1, 3, 1);
      break;
    case 2: // right leg
      currentSprite[0] = 3;
      currentSprite[1] = 3;
      printBitmap(5, 5, currentSprite);

      // crumbs
      randomDots(4, 8, 5, 8, 1, 5, 1);
      break;
    case 3: // left leg
      currentSprite[0] = 3;
      currentSprite[1] = 3;
      printBitmap(0, 5, currentSprite);

      // crumbs
      randomDots(0, 4, 5, 8, 2, 5, 1);
      break;
    case 4: // left arm
      currentSprite[0] = 3;
      currentSprite[1] = 2;
      // close up torso
      currentSprite[2 + 2] = B00000011; // row 2
      printBitmap(0, 2, currentSprite);
      // reset blank
      currentSprite[2 + 2] = 0;

      // crumbs
      randomDots(0, 2, 1, 5, 1, 3, 1);
      break;
    case 5: // torso
      currentSprite[0] = 4;
      currentSprite[1] = 4;
      printBitmap(2, 2, currentSprite);

      // crumbs
      randomDots(1, 7, 1, 7, 2, 6, 1);
      break;
  } // switch

  // check if newParts have all parts checked, then update state
  if (parts == B00111111) state = 2;
}


void animationHo(bool newAnimation) {
  static int state;

  if (newAnimation) {
    state = 0;
    return;
  }

  switch (state) {
    // initial is with 1st HO at a random spot, and 1 pause
    case 0:
      break; // pause
    case 2:
      m.clear(); // clear 1st
      break;
    case 3:
      printBitmapRandomLoc(false); // print 2nd HO
      break;
    case 4:
      break; // pause
    case 5:
      m.clear(); // clear 2nd
      break;
    case 6:
      printBitmapRandomLoc(false); // print 3rd HO
      break;
    case 7:
      break;  // pause
    case 8:
      m.clear(); // clear 3rd
      break;
    case 9:
    case 10:
      break;  // pause 2x
    case 11:
      printBitmapRandomLoc(false); // print 1st HO
      break;
  }

  state++;
  if (state > 11) state = 0;
}


void animationBell(bool newAnimation) {
  static int state;

  if (newAnimation) {
    state = 0;
    return;
  }
  
  // using "10" instead of ROW_SIZE because don't want to lose last byte
  // update current sprite buffer from various PROGMEM
  switch (state) {
    case 1:
      memcpy_P(currentSprite, SPRITES + ROW_SIZE * c, 10);
      break;
    case 0:
      memcpy_P(currentSprite, BELL_SPRITES + 10 * 0, 10);
      break;
    case 2:
      memcpy_P(currentSprite, BELL_SPRITES + 10 * 1, 10);
      break;
  }

  printBitmap(spriteX, spriteY, currentSprite);
  
  state++;
  if (state > 2) state = 0;
}


void animationSnow(bool newAnimation) {
  if (newAnimation) return;

  /* @NOTE works but the effect is jaring
  int x = random(0, 9);
  switch (x) {
    case 0:
      m.shiftLeft(true); // rotate - probably won't notice
      break;
    case 1:
      m.shiftRight(true); // rotate - probably won't notice 
      break;
    default:
      break;
  }
  */
  
  // snow fall
  m.shiftDown(false);
  randomDots(0, 8, 0, 1, 1, random(5, 10), 1);
}


void animationWreath(bool newAnimation) {
  int tempX;
  int tempY;

  if (newAnimation) return;

  for (int i = 0; i < 5; i++ ) {
    tempX = random(1, 4);
    tempY = random(1, 4);
  
    if ((tempX + tempY) != 4) return;
  
    int q = random(1, 5);
    switch(q) {
      case 1: 
        // no change
        break;
      case 2: 
        tempX = 7 - tempX;
        break;
      case 3: 
        tempY = 7 - tempY;
        break;
      case 4: 
        tempX = 7 - tempX;
        tempY = 7 - tempY;
        break;
    }
  
    m.setDot(tempX, tempY, random(0,2));
  }
}


void animationTree(bool newAnimation) {
  static int state;
  const int interval = 4;
  int tempX;
  int tempY;

  if (newAnimation) {
    state = 0;
    memcpy_P(currentSprite, BLANK, 10); // copy in blank sprite just once per animation
    currentSprite[0] = 2;
    currentSprite[1] = 2;
    return;
  }

  // blink the tree lights
  for (int i = 0; i < 5; i++) {
    tempX = random(1, 7);
    tempY = random(1, 7);
    
    if ((tempX + tempY) < 6) continue; // top left
    if (((7 - tempX) + tempY) < 6) continue; // top right
    
    m.setDot(tempX, tempY, random(0,2));
  }

  // flicker the star
  if ((state % interval) == 0) {
    if ((state % (interval * 2)) == 0) {
      currentSprite[2 + 0] = B00000010; // row 0
      currentSprite[2 + 1] = B00000001; // row 1
    } else {
      currentSprite[2 + 0] = B00000001; // row 0
      currentSprite[2 + 1] = B00000010; // row 1
    }
    printBitmap(3, 0, currentSprite);
  }

  state++;
  if (state == (interval * 2)) state = 0;
}


void animationRudolph(bool newAnimation) {
  static int state;

  if (newAnimation) {
    memcpy_P(currentSprite, BLANK, 10); // copy in blank sprite just once per animation;
    currentSprite[0] = 2;
    currentSprite[1] = 2;
    state = 0;
    return;
  }

  // @TODO decrease off time - use state

  if (state % 2) {
    currentSprite[2 + 0] = B00000011; // row 0
    currentSprite[2 + 1] = B00000011; // row 1
  } else {
    currentSprite[2 + 0] = B00000000; // row 0
    currentSprite[2 + 1] = B00000000; // row 1
  }
  printBitmap(3, 6, currentSprite);

  state = 1 - state; // toggles between 0 and 1
}


int valueBias1() {
  return (1 - random(0, random(0,3)));
}


int valueBias0() {
  return random(0, random(0,3));
}


void animationCandle(bool newAnimation) {
  const int offset = 2; // position of candle
  
  if (newAnimation) return;

  // leave as is because timing adds to effect?
  m.setDot(spriteX + offset + 1, 0, valueBias1());
  m.setDot(spriteX + offset + 0, 1, valueBias1());
  m.setDot(spriteX + offset + 1, 1, valueBias1());
  m.setDot(spriteX + offset + 2, 1, valueBias1());
  m.setDot(spriteX + offset + 1, 2, valueBias1());
}


void printNewSprite() {
  m.clear();

  // update current sprite buffer from PROGMEM
  memcpy_P(currentSprite, SPRITES + ROW_SIZE * c, ROW_SIZE);
  
  // verify size + sprite x & y fit!
  if ((currentSprite[0] + spriteX) > 8) spriteX = 0;
  if ((currentSprite[1] + spriteY) > 8) spriteY = 0;
  
  printBitmap(spriteX, spriteY, currentSprite);

  nextBitmapChange = t + BITMAP_CHANGE_INT;
}


void printBitmap(int x, int y, const unsigned char* b) {
  m.writeSprite(x, y, b);
}


void printBitmapRandomWalkReq() {
  int tempX;
  int tempY;
  
  do {
    // shortcut to 0 if size is 8 with ternary
    tempX = currentSprite[0] == 8 ? 0 : spriteX + random(0, 3) - 1; // randon yeilds -1, 0, 1
    tempY = currentSprite[1] == 8 ? 0 : spriteY + random(0, 3) - 1; // random yeilds -1, 0, 1
  } while ((tempX == spriteX && tempY == spriteY) ||
          tempX < 0 ||
          tempY < 0 ||
          (currentSprite[0] + tempX) > 8 ||
          (currentSprite[1] + tempY) > 8);

  spriteX = tempX;
  spriteY = tempY;
  
  // erase is assumed since something changed
  m.clear();
  // print requested character
  printBitmap(spriteX, spriteY, currentSprite);
}


void printBitmapRandomWalk() {
  int tempX = spriteX + random(0, 3) - 1; // random yeilds -1, 0, 1
  int tempY = spriteY + random(0, 3) - 1; // random yeilds -1, 0, 1

  if (tempX < 0) tempX = 0;
  else if ((tempX + currentSprite[0]) > 8 ) tempX = 8 - currentSprite[0];
  
  if (tempY < 0) tempY = 0;
  else if ((tempY + currentSprite[1]) > 8 ) tempY = 8 - currentSprite[1];
  
  if (tempX == spriteX && tempY == spriteY) return; // no change, so do nothing to reduce flicker

  spriteX = tempX;
  spriteY = tempY;
  
  // erase is assumed since something changed
  m.clear();
  // print requested character
  printBitmap(spriteX, spriteY, currentSprite);
}


void printBitmapRandomLoc(bool shortCircuit = true, bool erase = true) {
  // random (min inclusive, max exclusive)
  int tempX = random(0, 9 - currentSprite[0]);
  int tempY = random(0, 9 - currentSprite[1]);

  if (shortCircuit && tempX == spriteX && tempY == spriteY) return; // no change

  spriteX = tempX;
  spriteY = tempY;

  // erase is assumed since something changed
  if (erase == true) m.clear();

  // print requested character
  printBitmap(spriteX, spriteY, currentSprite);
};


int debouceButton(int pin) {
  static unsigned long nextButtonDebounceTime;
  static int lastButtonValue = HIGH; // pin is in a pullup state
  static int lastReadValue = HIGH; // ditto
  
  int b = digitalRead(pin);

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


enum buttonAction_e buttonCheckState() {
  static unsigned long buttonPressedTime = 0;
  static unsigned long buttonDepressedTime = 0;
  static int lastButtonValue = HIGH; // pin is in a pullup state
  enum state_e {Normal, firstPress, firstDepress};
  static enum state_e state = Normal; 
  
  // @NOTE enum buttonAction_e {None, Short, Long, veryLong, Double, Hold};
  // static buttonAction_e buttonAction = None;
  // const int shortButtonPressDur = 100;
  // const int longButtonPressDur = 1000;
  // @TODO double and hold times?
  
  int buttonValue = debouceButton(BUTTON_PIN);

  // @TODO check if doing "hold" would check time here, and next cond check "state == Normal"

  if (buttonValue != lastButtonValue) {
    lastButtonValue = buttonValue;
    
    switch (state) {
      case Normal:
        buttonPressedTime = t - BUTTON_DEBOUNCE_DELAY; // debounce only fires after BUTTON_DEBOUNCE_DELAY
        state = firstPress;
        return None;
      case firstPress:
        state = firstDepress;
        // @TODO if implementing double-press need to wait until the double-press intra-delay has elapsed
        // but for now change state and on next loop code in firstDepress will fire
        return None;
      case firstDepress:
        // if we're here on button change ... something's not right, but return None just in case
        return None;
    } // switch
  } else {
    // ie buttonValue == lastButtonValue
    switch (state) {
      case Normal:
        // nothing to do here
        return None;
      case firstPress:
        // nothing to do here
        return None;
      case firstDepress:
        state = Normal; // until double-press implemented
        // check for longest delay first 
        if (t > (buttonPressedTime + BUTTON_PRESS_VERY_LONG_DELAY)) {
          flashIntensity();
          flashIntensity();
          return veryLong;
        }
        if (t > (buttonPressedTime + BUTTON_PRESS_LONG_DELAY)) { // was longButtonPressDur
          flashIntensity();
          return Long;
        } else if (t > (buttonPressedTime + BUTTON_PRESS_SHORT_DELAY)) {  // was shortButtonPressDur
          return Short;
        }
        
        // if haven't exceeded either time, press was too short
        return None;
    } // switch
    
  } // if else
    
} // buttonCheckState()


void handleSpriteIndexChange() {
  if (c == MAX_SPRITES) c = 0;
    printNewSprite();
    nextAnimationChange = 0;
    nextBitmapChange = 0;

    // check if this has an animation function
    if (animations[c] != NULL) {
       // set initial animation timer
       (*animations[c])(true); // call animation function as "new" to setup
       nextAnimationChange = t + animationsInt[c];
    }
}


void flashIntensity() {
  // @TODO these delays probably not a good idea... mostly a @DEBUG idea
  m.setIntensity(0);
  delay(20);
  m.setIntensity(15);
  delay(20);
  m.setIntensity(intensity);
}


void normalLoop() {

  if (appState == Normal) {
    // enum buttonAction_e {None, Short, Long, Double, Hold} buttonState;
    switch (buttonState) {
      case Short:
        c++;
        handleSpriteIndexChange();
        break;
      case Long:
        flashIntensity();
        
        appState = autoAdvance;
        nextAutoAdvanceChange = t + AUTO_ADVANCE_CHANGE_INT;
        return;
      default:
        break; // fail-safe?
    } // switch
  } // if
  
  
  if (t > nextBitmapChange) {
    // reset timer
    nextBitmapChange = t + BITMAP_CHANGE_INT;

    // do action
    switch ((int)currentSprite[10]) {
      case ANI_RANDOM:
        printBitmapRandomLoc();
        break;
      case ANI_WALK_REQ:
        printBitmapRandomWalkReq();
        break;
      case ANI_WALK:
        printBitmapRandomWalk();
        break;
      case ANI_NONE:
      case ANI_CUSTOM:
      default:
        break; // do nothing
    }
    
  } // if

  // check if this is an animated sprite and the time has elapsed
  if ((animations[c] != NULL) && (t > nextAnimationChange)) {
    nextAnimationChange = t + animationsInt[c];
    (*animations[c])(false); // call the function pointed to in the animations table
  }
  
} // normalLoop


void autoAdvanceLoop() {
  
  // enum buttonAction_e {None, Short, Long, Double, Hold} buttonState;
  switch (buttonState) {
    case Short:
      break; // do nothing
    case Long:
      flashIntensity();
      
      nextAutoAdvanceChange = 0;
      appState = shuffleAdvance;
      return;
    default:
      break;
  } // switch

  if (t > nextAutoAdvanceChange) {
    // reset timer
    nextAutoAdvanceChange = t + AUTO_ADVANCE_CHANGE_INT;

    c++; // go to next sprite
    handleSpriteIndexChange();
  }

  normalLoop();
  
} // autoAdvanceLoop


void shuffleAdvanceLoop() {
  
  // enum buttonAction_e {None, Short, Long, Double, Hold} buttonState;
  switch (buttonState) {
    case Short:
      break; // do nothing
    case Long:
      flashIntensity();
      
      nextAutoAdvanceChange = 0;
      appState = Normal;
      return;
    default:
      break;
  } // switch

  if (t > nextAutoAdvanceChange) {
    // reset timer
    nextAutoAdvanceChange = t + AUTO_ADVANCE_CHANGE_INT;

    // go to next sprite
    c = random(0, MAX_SPRITES);
    handleSpriteIndexChange();
  }

  normalLoop();
  
} // autoAdvanceLoop


void adminLoop(bool adminInit) {
  static enum appState_e oldAppState;
  static int direction = 1;
  
  if (adminInit) {
    oldAppState = appState;
    appState = Admin;
  }

  // enum buttonAction_e {None, Short, Long, Double, Hold} buttonState;
  switch (buttonState) {
    case Short:
      intensity += direction;
      if (intensity > 15) {
        intensity = 0;
      } else if (intensity < 0) {
        intensity = 15;
      }
      m.setIntensity(intensity);
      break;
    case Long:
      direction = 0 - direction; // "reverse" it
      break;
    case veryLong:
      flashIntensity();
      flashIntensity();
      
      appState = oldAppState;
      return;
  }

}


void loop() {
  t = millis();

  buttonState = buttonCheckState();

  if (buttonState == veryLong && appState != Admin) {
    buttonState = None;
    adminLoop(true);
  }
  

  // @NOTE enum appState_e {Normal, autoAdvance, Admin};
  switch (appState) {
    case Normal:
      normalLoop();
      break;
    case autoAdvance:
      autoAdvanceLoop();      
      break;
    case shuffleAdvance:
      shuffleAdvanceLoop();
      break;
    case Admin:
      adminLoop(false);
      break;
    default:
      appState = Normal; // @NOTE fail-safe?
      break;
  }
  
  delay(1);
}

