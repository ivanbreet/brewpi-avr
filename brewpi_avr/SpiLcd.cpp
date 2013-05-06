/*
 * Copyright 2012 BrewPi/Elco Jacobs.
 *
 * This file is part of BrewPi.
 * 
 * BrewPi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * BrewPi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with BrewPi.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SpiLcd.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <util/delay.h>
#include <util/atomic.h>

void SpiLcd::init(uint8_t latchPin)
{
	_latchPin = latchPin;
	pinMode(_latchPin, OUTPUT);
	
	_displayfunction = LCD_FUNCTIONSET | LCD_4BITMODE;

	initSpi();   
		
	for(uint8_t i = 0; i<4; i++){
		for(uint8_t j = 0; j<20; j++){
			content[i][j]=' '; // initialize on all spaces
		}
		content[i][20]='\0'; // NULL terminate string
	}
	_backlightTime = 0;
}

void SpiLcd::begin(uint8_t cols, uint8_t lines) {
	_numlines = lines;
	_currline = 0;
	_currpos = 0;
  
	// Set all outputs of shift register to low, this turns the backlight ON.	
	// The following initialization sequence should be compatible with: 
	// - Newhaven OLED displays
	// - Standard HD44780 or S6A0069 LCD displays
	delayMicroseconds(50000); // wait 50 ms just to be sure that the lcd is initialized

	write4bits(0x03); //set to 8-bit
	delayMicroseconds(50000); // wait > 4.1ms
	write4bits(0x03); //set to 8-bit
	delayMicroseconds(1000); // wait > 100us
	write4bits(0x03); //set to 8-bit
	delayMicroseconds(50000); // wait for execution
	write4bits(0x02); //set to 4-bit
	delayMicroseconds(50000); // wait for execution
	command(0x28); // set to 4-bit, 2-line
		
	clear();	// display clear
	// Entry Mode Set:
	leftToRight();
	noAutoscroll();
	
	home();
	
	//noCursor();
	display();
}

/********** high level commands, for the user! */
void SpiLcd::clear()
{
	command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
}

void SpiLcd::home()
{
	command(LCD_RETURNHOME);  // set cursor position to zero
	_currline = 0;
	_currpos = 0;
}

void SpiLcd::setCursor(uint8_t col, uint8_t row)
{
	uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row >= _numlines ) {
		row = 0;  //write to first line if out off bounds
	}
	_currline = row;
	_currpos = col;
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void SpiLcd::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void SpiLcd::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void SpiLcd::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void SpiLcd::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void SpiLcd::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void SpiLcd::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void SpiLcd::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void SpiLcd::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void SpiLcd::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void SpiLcd::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void SpiLcd::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void SpiLcd::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void SpiLcd::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

// This resets the backlight timer and updates the SPI output
void SpiLcd::resetBacklightTimer(void){
	_backlightTime = ticks.seconds();
	updateBacklight();
	spiOut();
}

void SpiLcd::updateBacklight(void){
	bool backLightOutput = ticks.timeSince(_backlightTime) > BACKLIGHT_AUTO_OFF_PERIOD;
	bitWrite(_spiByte, LCD_SHIFT_BACKLIGHT, backLightOutput); // 1=OFF, 0=ON
}

// Puts the content of one LCD line into the provided buffer.
void SpiLcd::getLine(uint8_t lineNumber, char * buffer){
	for(uint8_t i =0;i<20;i++){
		if(content[lineNumber][i] == 0b11011111){
			buffer[i] = 0xB0; // correct degree sign
		}
		else{
			buffer[i] = content[lineNumber][i]; // copy to string buffer
		}
	}
	buffer[20] = '\0'; // NULL terminate string
}

/*********** mid level commands, for sending data/cmds */

inline void SpiLcd::command(uint8_t value) {
	send(value, LOW);
	waitBusy();
}

inline size_t SpiLcd::write(uint8_t value) {
	send(value, HIGH);
	content[_currline][_currpos] = value;
	_currpos++;
	waitBusy();
	return 1;
}

/************ low level data pushing commands **********/
void SpiLcd::initSpi(void){
	// Set MOSI and CLK to output
	pinMode(MOSI, OUTPUT);
	pinMode(SCK, OUTPUT);
	pinMode(SS, OUTPUT);
	// The most significant bit should be sent out by the SPI port first.
	// equals SPI.setBitOrder(MSBFIRST);
	SPCR &= ~_BV(DORD);

	// Set the SPI clock to Fosc/128, the slowest option. This prevents problems with long cables.
	SPCR |= _BV(SPR1);
	SPSR |= _BV(SPR0);

	// Set clock polarity and phase for shift registers (Mode 3)
	SPCR |= _BV(CPOL);
	SPCR |= _BV(CPHA);

	// Warning: if the SS pin ever becomes a LOW INPUT then SPI
	// automatically switches to Slave, so the data direction of
	// The SS pin MUST be kept as OUTPUT.	
	// On the Arduino Leonardo it is connected to the RX status LED, on the Uno we are using it as latch pin.
	
	// Set SPI as master and enable SPI
	SPCR |= _BV(MSTR);
	SPCR |= _BV(SPE);
}

// Update the pins of the shift register
void SpiLcd::spiOut(void){
	digitalWrite(_latchPin, LOW);
	SPDR = _spiByte; // Send the byte to the SPI
	// wait for send to finish
	while (!(SPSR & _BV(SPIF))); 
	
	digitalWrite(_latchPin, HIGH);
}

// write either command or data
void SpiLcd::send(uint8_t value, uint8_t mode) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE){ // prevent interrupts during command
		if(mode){
			bitSet(_spiByte, LCD_SHIFT_RS);
		}
		else{
			bitClear(_spiByte, LCD_SHIFT_RS);
		}
		spiOut();
		write4bits(value>>4);
		write4bits(value);	
	}
}

void SpiLcd::pulseEnable(void) {
	bitSet(_spiByte, LCD_SHIFT_ENABLE);
	spiOut();
	delayMicroseconds(1); // enable pulse must be >450ns
	bitClear(_spiByte, LCD_SHIFT_ENABLE);
	spiOut();
}

void SpiLcd::write4bits(uint8_t value) {	
	_spiByte = (_spiByte & ~LCD_SHIFT_DATA_MASK) | (value << 4);
	spiOut();
	pulseEnable();
}

void SpiLcd::waitBusy(void) {
	// we cannot read the busy pin, so just wait 1 ms
	_delay_ms(1);
}