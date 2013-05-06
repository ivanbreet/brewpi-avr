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

#ifndef C_lcd_H_
#define C_lcd_H_

#include "pins.h"
#include "SpiLcd.h"
#include "temperatureFormats.h"

class Display{
	public:
	Display(){};
	~Display(){};
		
	// initializes the lcd display
	void init(void);
			
	// print all temperatures on the LCD
	void printAllTemperatures(void);
	
	// print a temperature
	void printTemperature(fixed7_9 temp);
	
	// print " --.-"
	void printUndefinedTemperature(void);

	// print the stationary text on the lcd.
	void printStationaryText(void);

	// print degree sign + C/F
	void printDegreeUnit(void);

	// print mode on the right location on the first line, after Mode:
	void printMode(void);

	// print beer temperature at the right place on the display
	void printBeerTemp(void);

	// print beer temperature setting at the right place on the display
	void printBeerSet(void);

	// print fridge temperature at the right place on the display
	void printFridgeTemp(void);

	// print fridge temperature setting at the right place on the display
	void printFridgeSet(void);

	// print the current state on the last line of the LCD
	void printState(void);

	// print the static text of a menu page
	void printMenu(int menuPage);

	void readContent(void);
		
	public:
	SpiLcd lcd;
			
	private:
	uint8_t stateOnDisplay;
};

extern Display display;

#endif /* C_lcd_H_ */