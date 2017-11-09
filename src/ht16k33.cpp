/*
 * ht16k33.cpp - used to talk to the htk1633 chip to do things like turn on LEDs or scan keys
 * Copyright:  Peter Sjoberg <peters-alib AT techwiz.ca>
 * License: GPLv3
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as 
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * History:
 * 2015-10-04  Peter Sjoberg <peters-alib AT techwiz.ca>
 *             Created using https://www.arduino.cc/en/Hacking/LibraryTutorial and ht16k33 datasheet
 * 2015-11-25  Peter Sjoberg <peters-alib AT techwiz DOT ca>
 *	       	   First check in to github
 * 2016-08-09  René Wennekes <rene.wennekes AT gmail.com>
 *             Contribution of 7-segment & 16-segment display support
 *             Added clearAll() function
 * 2017-11-09  Victor Nieto <victornt95 AT gmail.com>
 *			   Optimization and simplification. Adapted to use on 1.2" 7-segment LED HT16K33 Backpack
 *
 */

#include "Arduino.h"
#include "ht16k33.h"
#include <Wire.h>

// "address" is base address 0-7 which becomes 11100xxx = E0-E7
#define BASEHTADDR 0x70

//Commands
#define HT16K33_DDAP          B00000000 // Display data address pointer: 0000xxxx
#define HT16K33_SS            B00100000 // System setup register
#define HT16K33_SS_STANDBY    B00000000 // System setup - oscillator in standby mode
#define HT16K33_SS_NORMAL     B00000001 // System setup - oscillator in normal mode
#define HT16K33_DSP           B10000000 // Display setup
#define HT16K33_DSP_OFF       B00000000 // Display setup - display off
#define HT16K33_DSP_ON        B00000001 // Display setup - display on
#define HT16K33_DSP_NOBLINK   B00000000 // Display setup - no blink
#define HT16K33_DSP_BLINK2HZ  B00000010 // Display setup - 2hz blink
#define HT16K33_DSP_BLINK1HZ  B00000100 // Display setup - 1hz blink
#define HT16K33_DSP_BLINK05HZ B00000110 // Display setup - 0.5hz blink
#define HT16K33_RIS           B10100000 // ROW/INT Set
#define HT16K33_RIS_OUT       B00000000 // Set INT as row driver output
#define HT16K33_RIS_INTL      B00000001 // Set INT as int active low
#define HT16K33_RIS_INTH      B00000011 // Set INT as int active high
#define HT16K33_DIM           B11100000 // Dimming set
#define HT16K33_DIM_1         B00000000 // Dimming set - 1/16
#define HT16K33_DIM_2         B00000001 // Dimming set - 2/16
#define HT16K33_DIM_3         B00000010 // Dimming set - 3/16
#define HT16K33_DIM_4         B00000011 // Dimming set - 4/16
#define HT16K33_DIM_5         B00000100 // Dimming set - 5/16
#define HT16K33_DIM_6         B00000101 // Dimming set - 6/16
#define HT16K33_DIM_7         B00000110 // Dimming set - 7/16
#define HT16K33_DIM_8         B00000111 // Dimming set - 8/16
#define HT16K33_DIM_9         B00001000 // Dimming set - 9/16
#define HT16K33_DIM_10        B00001001 // Dimming set - 10/16
#define HT16K33_DIM_11        B00001010 // Dimming set - 11/16
#define HT16K33_DIM_12        B00001011 // Dimming set - 12/16
#define HT16K33_DIM_13        B00001100 // Dimming set - 13/16
#define HT16K33_DIM_14        B00001101 // Dimming set - 14/16
#define HT16K33_DIM_15        B00001110 // Dimming set - 15/16
#define HT16K33_DIM_16        B00001111 // Dimming set - 16/16

static const uint8_t seg7FontTable[]  = {
  0b00111111, // 0
  0b00110000, // 1
  0b01011011, // 2
  0b01111001, // 3
  0b01110100, // 4
  0b01101101, // 5
  0b01101111, // 6
  0b00111000, // 7
  0b01111111, // 8
  0b01111100, // 9
  0b00000000  // space
};

// Constructor
HT16K33::HT16K33(){
	_seg7Font=(uint8_t*) &seg7FontTable;
}

/****************************************************************/
// Setup the env
//
void HT16K33::begin(uint8_t address){
  uint8_t i;
  _address=address | BASEHTADDR;
  Wire.begin();
  Wire.setClock(1000000L); // Specs set max 400kHZ, but my testing backpack supports this mode without problems
  i2c_write(HT16K33_SS  | HT16K33_SS_NORMAL); // Wakeup
  i2c_write(HT16K33_DSP | HT16K33_DSP_ON | HT16K33_DSP_NOBLINK); // Display on and no blinking
  i2c_write(HT16K33_RIS | HT16K33_RIS_OUT); // INT pin works as row output 
  i2c_write(HT16K33_DIM | HT16K33_DIM_16);  // Brightness set to max
  clearAll();
} // begin

/****************************************************************/
// internal function - Write a single byte
//
uint8_t HT16K33::i2c_write(uint8_t val){
  Wire.beginTransmission(_address);
  Wire.write(val);
  return Wire.endTransmission();
} // i2c_write

/****************************************************************/
// internal function - Write several bytes
// "size" is amount of data to send excluding the first command byte
// if LSB is true then swap high and low byte to send LSB MSB
// NOTE: Don't send odd amount of data if using LSB, then it will send one to much
//
uint8_t HT16K33::i2c_write(uint8_t cmd,uint8_t *data,uint8_t size,boolean LSB){
  uint8_t i;
  Wire.beginTransmission(_address);
  Wire.write(cmd);
  i=0;
  while (i<size){
    if (LSB){
      Wire.write(data[i+1]);
      Wire.write(data[i++]);
      i++;
    } else {
      Wire.write(data[i++]);
    }
  }
  return Wire.endTransmission(); // Send out the data
} // i2c_write

/****************************************************************/
// internal function - read a byte from specific address (send one byte(address to read) and read a byte)
//
uint8_t HT16K33::i2c_read(uint8_t addr){
  i2c_write(addr);
  Wire.requestFrom(_address,(uint8_t) 1);
  return Wire.read();    // read one byte
} // i2c_read

/****************************************************************/
// read an array from specific address (send a byte and read several bytes back)
// return value is how many bytes that where really read
//
uint8_t HT16K33::i2c_read(uint8_t addr,uint8_t *data,uint8_t size){
  uint8_t i,retcnt,val;
  
  i2c_write(addr);
  retcnt=Wire.requestFrom(_address, size);
  i=0;
  while(Wire.available() && i<size)    // slave may send less than requested
  {
    data[i++] = Wire.read();    // receive a byte as character
  }

  return retcnt;
} // i2c_read

/****************************************************************/
// Clear all leds and displays
//
void HT16K33::clearAll(){
  memset(displayRam,0,sizeof(displayRam));
  i2c_write(HT16K33_DDAP, displayRam,sizeof(displayRam));
} // clearAll

/****************************************************************/
// Put the chip to sleep
//
uint8_t HT16K33::sleep(){
  return i2c_write(HT16K33_SS|HT16K33_SS_STANDBY); // Stop oscillator
} // sleep

/****************************************************************/
// Wake up the chip (after it been a sleep)
//
uint8_t HT16K33::normal(){
  return i2c_write(HT16K33_SS|HT16K33_SS_NORMAL); // Start oscillator
} // normal

/****************************************************************/
// Turn off one led but only in memory
// To do it on chip a call to "sendLed" is needed
//
uint8_t HT16K33::clearLed(uint8_t ledno){ // 16x8 = 128 LEDs to turn on, 0-127
  if (ledno>=0 && ledno<128){
    bitClear(displayRam[int(ledno/8)],(ledno % 8));
    return 0;
  } else {
    return 1;
  }
} // clearLed

/****************************************************************/
// Turn on one led but only in memory
// To do it on chip a call to "sendLed" is needed
//
uint8_t HT16K33::setLed(uint8_t ledno){ // 16x8 = 128 LEDs to turn on, 0-127
  if (ledno>=0 && ledno<128){
    bitSet(displayRam[int(ledno/8)],(ledno % 8));
    return 0;
  } else {
    return 1;
  }
} // setLed

/****************************************************************/
// check if a specific led is on(true) or off(false)
//
boolean HT16K33::getLed(uint8_t ledno,boolean Fresh){ 

  // get the current state from chip
  if (Fresh) {
    i2c_read(HT16K33_DDAP, displayRam,sizeof(displayRam));
  }

  if (ledno>=0 && ledno<128){
    return bitRead(displayRam[int(ledno/8)],ledno % 8) != 0;
  }
} // getLed

/****************************************************************/
uint8_t HT16K33::setDisplayRaw(uint8_t pos, uint8_t val) {
  if (pos < sizeof(displayRam)) {
    displayRam[pos] = val;
    return 0;
  } else {
    return 1;
  }
} // setDisplayRaw

/****************************************************************/
// Send the display ram info to chip - kind of commit all changes to the outside world
//
uint8_t HT16K33::sendLed(){
  return i2c_write(HT16K33_DDAP, displayRam,sizeof(displayRam));
} // sendLed

/****************************************************************/
// Turn on one 7-segment but only in memory
// To do it on chip a call to "sendLed" is needed
//
uint8_t HT16K33::set7Seg(uint8_t dig, uint8_t cha){ // position 0-15, 0-15 (0-F Hexadecimal)
  if (cha>=0 && cha<16 && dig>=0 && dig<16){
    uint8_t num = _seg7Font[cha];
    displayRam[dig] = num;
    return 0;
  } else {
    return 1;
  }
} // set7Seg

/****************************************************************/
// Change brightness of the whole display
// level 0-15, 0 means display off
//
uint8_t HT16K33::setBrightness(uint8_t level){
  if (HT16K33_DIM_1>=0 && level <HT16K33_DIM_16){
    return i2c_write(HT16K33_DIM|level);
  } else {
    return 1;
  }
} // setBrightness

/****************************************************************/
// Make the display blink
//
uint8_t HT16K33::setBlinkRate(uint8_t rate){
  switch (rate) {
    case HT16K33_DSP_NOBLINK:
    case HT16K33_DSP_BLINK2HZ:
    case HT16K33_DSP_BLINK1HZ:
    case HT16K33_DSP_BLINK05HZ:
      i2c_write(HT16K33_DSP |rate);
      return 0;
      ;;
  default:
    return 1;
  }
} //setBlinkRate

/****************************************************************/
// turn on the display
//
void HT16K33::displayOn(){
  i2c_write(HT16K33_DSP|HT16K33_DSP_ON);
} // displayOn

/****************************************************************/
// turn off the display
//
void HT16K33::displayOff(){
  i2c_write(HT16K33_DSP|HT16K33_DSP_OFF);
} // displayOff


