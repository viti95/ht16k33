/****************************************************************
   Simple test of ht16k33 library turning on and off LEDs
*/

#include "ht16k33.h"

#define DIGIT_1 0
#define DIGIT_2 2
#define DIGIT_3 6
#define DIGIT_4 8

// Define the class
HT16K33 HT;

int ExtractDigit(int V, int P)
{
  unsigned long Pow = 1;
  for(byte D= 0; D < (P-1); D++)
    Pow *= 10;

  return (V / Pow) % 10;
}
/****************************************************************/
void setup() {
  HT.begin(0x00);
}

/****************************************************************/
void loop() {
  uint8_t led;
  uint8_t brightness;
  uint16_t counter;

  // first light up all LEDs
  for (led = 0; led < 128; led++) {
    HT.setLed(led);    
  } // for led
  HT.sendLed();

  delay(2000);

  //Next clear them
  for (led = 0; led < 128; led++) {
    HT.clearLed(led);    
  } // for led
  HT.sendLed();
  
  delay(2000);

  for (led = 0; led < 128; led++) {
    HT.setLed(led);
  } // for led
  HT.sendLed();
  
  for (brightness = 0; brightness < 16; brightness++) {
    HT.setBrightness(brightness);
    delay(500);
  } // for led

  HT.clearAll();
  //Now do one by one, slowly, and print out which one
  for (led = 0; led < 128; led++) {
    HT.setLed(led);
    HT.sendLed();
    delay(50);
    HT.clearLed(led);
    HT.sendLed();
  } // for led*/

  for (counter = 0; counter < 10000; counter++){
    HT.set7Seg(DIGIT_1, ExtractDigit(counter, 1));
    HT.set7Seg(DIGIT_2, ExtractDigit(counter, 2));
    HT.set7Seg(DIGIT_3, ExtractDigit(counter, 3));
    HT.set7Seg(DIGIT_4, ExtractDigit(counter, 4));
    HT.sendLed();
    delay(25);
  }

  delay(2000);

} // loop

