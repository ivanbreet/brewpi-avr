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

#include "Ticks.h"


// return time that has passed since timeStamp, take overflow into account
ticks_seconds_t Ticks::timeSince(ticks_seconds_t previousTime){
	ticks_seconds_t currentTime = ticks.seconds();
	if(currentTime>=previousTime){
		return currentTime - previousTime;
	}
	else{
		// overflow has occurred
		return (currentTime + 1440) - (previousTime +1440); // add a day to both for calculation
	}
}

Ticks ticks;
Delay wait;