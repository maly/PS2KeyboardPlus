/*
  PS2KeyboardPlus.cpp - PS2KeyboardPlus library

  Copyright (c) 2016 Martin Maly <retrocip@maly.cz>

  Full PS/2 keyboard Arduino driver

  Featured:
  - read scancodes
  - you can set handlers for keyPress and keyRelease events
  - you can control the LEDs on the keyboard
  - you can select the CLK pin to pin 2 or 3

  TODO: 
  - Conversion from scancode to chars


  Based on PS2Keyboard by Christian Weichel <info@32leaves.net>
  Copyright (c) 2007 Free Software Foundation.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "Arduino.h"
#include "PS2KeyboardPlus.h"


#define BREAK  0xf0
#define EXT     0xe0


/*
 * I do know this is so uncool, but I just don't see a way arround it
 * REALLY BAD STUFF AHEAD
 *
 * The variables are used for internal status management of the ISR. The're
 * not kept in the object instance because the ISR has to be as fast as anyhow
 * possible. So the overhead of a CPP method call is to be avoided.
 *
 * PLEASE DO NOT REFER TO THESE VARIABLES IN YOUR CODE AS THEY MIGHT VANISH SOME
 * HAPPY DAY.
 */
int dataPin;
uint8_t inputBuffer;
uint8_t sendBuffer;
unsigned int extended;
volatile byte received;
uint8_t posReceive, posSend;
bool breakFlag;
unsigned int extendedFlag;
bool sendingFlag,waitToAck;
uint8_t parity;

KB_HANDLER onPress,onRelease;

// The ISR for the external interrupt
#if PS2_CLK_PIN==2
ISR(INT0_vect) {
#endif

#if PS2_CLK_PIN==3  
ISR(INT1_vect) {
#endif

  //------SEND BITS
  if (sendingFlag) {
    //send bits 0 to 7, compute a parity
    if(posSend >= 0 && posSend < 8) {
      if (sendBuffer & 1) {
        parity ^= 1;
        digitalWrite(dataPin, HIGH);
      } else {
        digitalWrite(dataPin, LOW);
      }
      sendBuffer >>= 1;
    }

    //send parity
    if(posSend == 8) {
      if (parity) {
        digitalWrite(dataPin, LOW);
      } else {
        digitalWrite(dataPin, HIGH);
      }
    }

    //parity is sent. Now back to the reality
    if(posSend == 9) {
      digitalWrite(dataPin, HIGH);
      pinMode(dataPin, INPUT);
    }


    //All done
    if(posSend == 11) {
      sendingFlag = false;
      posSend=0;
      return;
    }

    posSend ++;

    return;
  }

  //------RECEIVE BITS

  int bit = digitalRead(dataPin);
  
  if(posReceive > 0 && posReceive < 9) {
    inputBuffer |= (bit << (posReceive - 1));
  }
  
  posReceive++;
  
  if(posReceive == 11) {
    if(inputBuffer == EXT) {
      extendedFlag = 256;
    } else
      if(inputBuffer == BREAK) {
        breakFlag = true;
      } else 
        if(breakFlag) {
          breakFlag = false;
          onRelease(inputBuffer+extendedFlag);
          extendedFlag = 0;
        } else {
          received = inputBuffer;   
          extended = extendedFlag;   
          if (!waitToAck) onPress(inputBuffer+extendedFlag);
          extendedFlag = 0;

        }
    inputBuffer = 0;
    posReceive = 0;
  }
}

PS2KeyboardPlus::PS2KeyboardPlus() {;}


// dummy kb handler
void dummy(unsigned int a){;}

void PS2KeyboardPlus::begin(int data) {
  dataPin = data;
  inputBuffer = 0;
  received = 0;
  posReceive = 0;
  breakFlag = false;
  extendedFlag = 0;
  sendingFlag = false;
  waitToAck = false;
  onPress=dummy;
  onRelease=dummy;

  pinMode(PS2_CLK_PIN, INPUT_PULLUP);
  digitalWrite(PS2_CLK_PIN, HIGH);
  pinMode(dataPin, INPUT);
  digitalWrite(dataPin, HIGH);

#if PS2_CLK_PIN==3  
  EIMSK |= (1 << INT1);
  EICRA |= (0 << ISC10) | (1 << ISC11);
#endif  

#if PS2_CLK_PIN==2  
  EIMSK |= (1 << INT0);
  EICRA |= (0 << ISC00) | (1 << ISC01);
#endif  

 //send(0xff);
 //delay(1000);
}

void PS2KeyboardPlus::setPressHandler(KB_HANDLER handler) {
  onPress = handler;
}

void PS2KeyboardPlus::setReleaseHandler(KB_HANDLER handler) {
  onRelease = handler;
}


bool PS2KeyboardPlus::available() {
  return (received != 0 && !waitToAck);
}

void PS2KeyboardPlus::send(uint8_t byte) {
  sendingFlag = true;
  sendBuffer = byte;
  posSend = 0;
  parity=0;
  digitalWrite(dataPin, HIGH);
  pinMode(dataPin, OUTPUT);
  digitalWrite(PS2_CLK_PIN, HIGH);
  pinMode(PS2_CLK_PIN, OUTPUT);
  delayMicroseconds(10);
  digitalWrite(PS2_CLK_PIN, LOW);
  delayMicroseconds(100);
  digitalWrite(dataPin, LOW);
  delayMicroseconds(10);
  pinMode(PS2_CLK_PIN, INPUT_PULLUP);
}

void PS2KeyboardPlus::setLED(uint8_t status) {
  waitToAck = true;
  received = 0;
  send(0xED);
  while (received==0) {}
  send(status);
  received = 0;
  while (received==0) {}
  received=0;
  delay(50);
  inputBuffer = 0;
  posReceive = 0;
  waitToAck = false;
}

void PS2KeyboardPlus::reset() {
  waitToAck = true;
  send(0xFF);
  while (received!=0xaa) {}
  received = 0;
  waitToAck = false;
}

void PS2KeyboardPlus::disable() {
  waitToAck = true;
  send(0xf5);
  while (received==0x00) {}
  received = 0;
  waitToAck = false;
}

void PS2KeyboardPlus::enable() {
  waitToAck = true;
  send(0xf4);
  while (received==0) {}
  received = 0;
  waitToAck = false;
}


unsigned int PS2KeyboardPlus::read() {
  unsigned int result = received + extended;
  
  received = 0;
  
  return result;
}
