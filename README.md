# Arduino Holtek HT16K33 library
This library is used to allow an Arduino device talk to the Holtek HT16K33 chip
to control a 7-segment based display. Uses the I2C port and the Wired library.

![HT16K33 image](https://github.com/viti95/ht16k33/raw/master/img/ht16k33_example.jpg)

## Device connection

* Arduino 5v  -> Backpack +5v / +
* Arduino GND -> Backpack GND / -
* Arduino SDA -> Backpack SDA / D
* Arduino SCL -> Backpack SCL / C

## Installation

Copy master folder onto your Arduino library

## Usage

Simply import the file 'ht16k33.h' into your .ino

    #include "ht16k33.h"

And then define the class, plus initilize it on the setup

    HT16K33 HT;
    void setup(){
      HT.begin(0x00); // I2C address
    }

## Examples

It's included one complete example that demostrates how to use the library. Tested and working on this devices:
* Arduino Mega 2560
* Arduino Due
