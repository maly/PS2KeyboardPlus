/*
  PS2KeyboardPlus.h - PS2Keyboard library
  Copyright (c) 2007 Free Software Foundation.  All right reserved.
  Written by Christian Weichel <info@32leaves.net>

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


#ifndef PS2KeyboardPlus_h
#define PS2KeyboardPlus_h

typedef void (*KB_HANDLER) (unsigned int);

#include <avr/io.h>
#include <avr/interrupt.h>

#define PS2_LED_NONE   0x00
#define PS2_LED_SCROLL 0x01
#define PS2_LED_NUM    0x02
#define PS2_LED_CAPS   0x04

/*
 * This PIN is hardcoded in the init routine later on. 
 * You can use the pin 2 too
 */
#define PS2_CLK_PIN 3

/**
 * Purpose: Provides an easy access to PS2 keyboards
 * Author:  Christian Weichel
 */
class PS2KeyboardPlus {
  
  private:
  
  public:
  	/**
  	 * This constructor does basically nothing. Please call the begin(int)
  	 * method before using any other method of this class.
  	 */
  	PS2KeyboardPlus();
    
    /**
     * Starts the keyboard "service" by registering the external interrupt.
     * setting the pin modes correctly and driving those needed to high.
     * The propably best place to call this method is in the setup routine.
     */
    void begin(int data);

    /**
     * Sets the keyboard press / release handlers. It gets scancode as parameter
     */

    void setPressHandler(KB_HANDLER handler);

    void setReleaseHandler(KB_HANDLER handler);

    
    /**
     * Returns true if there is a char to be read, false if not.
     */
    bool available();
    
    /**
     * Returns the scancode last read from the keyboard. If the user has pressed two
     * keys between calls to this method, only the later one will be availble. Once
     * the scancode has been read, the buffer will be cleared.
     * If there is no scancode availble, 0 is returned.
     */
    unsigned int read();

    /**
     * Send a command to the keyboard
     */
    void send(uint8_t byte);

    /**
     * Set LED controls
     * Use PS2_LED_xxx constants to set appropriate LED light
     */
    void setLED(uint8_t status);

    void reset();
    void disable();
    void enable();

    
};

#endif
