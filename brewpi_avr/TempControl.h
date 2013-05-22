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


#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "Brewpi.h"
#include "TempSensor.h"
#include "Pins.h"
#include "TemperatureFormats.h"
#include "Actuator.h"
#include "Sensor.h"
#include "EepromManager.h"
#include "ActuatorAutoOff.h"


// Set minimum off time to prevent short cycling the compressor in seconds
#define MIN_COOL_OFF_TIME 300u
// Use a minimum off time for the heater as well, so it heats in cycles, not lots of short bursts
#define MIN_HEAT_OFF_TIME 300u
// Minimum on time for the cooler.
#define MIN_COOL_ON_TIME 300u
// Minimum on time for the heater.
#define MIN_HEAT_ON_TIME 300u
// Use a large minimum off time in fridge constant mode. No need for very fast cycling.
#define MIN_COOL_OFF_TIME_FRIDGE_CONSTANT 900u
// Set a minimum off time between switching between heating and cooling
#define MIN_SWITCH_TIME 600u
// Time allowed for peak detection
#define COOL_PEAK_DETECT_TIME 1800u
#define HEAT_PEAK_DETECT_TIME 900u

// These two structs are stored in and loaded from EEPROM
struct ControlSettings{
	char mode;
	fixed7_9 beerSetting;
	fixed7_9 fridgeSetting;
	fixed7_9 heatEstimator; // updated automatically by self learning algorithm
	fixed7_9 coolEstimator; // updated automatically by self learning algorithm
};

struct ControlVariables{
	fixed7_9 beerDiff;
	fixed23_9 diffIntegral; // also uses 9 fraction bits, but more integer bits to prevent overflow
	fixed7_9 beerSlope;
	fixed7_9 p;
	fixed7_9 i;
	fixed7_9 d;
	fixed7_9 estimatedPeak;
	fixed7_9 negPeakEstimate; // last estimate
	fixed7_9 posPeakEstimate;
	fixed7_9 negPeak; // last detected peak
	fixed7_9 posPeak;
};

struct ControlConstants{
	char tempFormat;
	fixed7_9 tempSettingMin;
	fixed7_9 tempSettingMax;	
	fixed7_9 Kp;
	fixed7_9 Ki;
	fixed7_9 Kd;
	fixed7_9 iMaxError;
	fixed7_9 idleRangeHigh;
	fixed7_9 idleRangeLow;
	fixed7_9 heatingTargetUpper;
	fixed7_9 heatingTargetLower;
	fixed7_9 coolingTargetUpper;
	fixed7_9 coolingTargetLower;
	uint16_t maxHeatTimeForEstimate; // max time for heat estimate in seconds
	uint16_t maxCoolTimeForEstimate; // max time for heat estimate in seconds
	// for the filter coefficients the b value is stored. a is calculated from b.
	uint8_t fridgeFastFilter;	// for display, logging and on-off control
	uint8_t fridgeSlowFilter;	// for peak detection
	uint8_t fridgeSlopeFilter;	// not used in current control algorithm
	uint8_t beerFastFilter;	// for display and logging
	uint8_t beerSlowFilter;	// for on/off control algorithm
	uint8_t beerSlopeFilter;	// for PID calculation
	uint8_t lightAsHeater;		// use the light to heat rather than the configured heater device
};

#define EEPROM_TC_SETTINGS_BASE_ADDRESS 0
#define EEPROM_CONTROL_SETTINGS_ADDRESS (EEPROM_TC_SETTINGS_BASE_ADDRESS+sizeof(uint8_t))
#define EEPROM_CONTROL_CONSTANTS_ADDRESS (EEPROM_CONTROL_SETTINGS_ADDRESS+sizeof(ControlSettings))

#define	MODE_FRIDGE_CONSTANT 'f'
#define MODE_BEER_CONSTANT 'b'
#define MODE_BEER_PROFILE 'p'
#define MODE_OFF 'o'
#define MODE_TEST 't'

enum states{
	IDLE,
	STARTUP,
	STATE_OFF,
	DOOR_OPEN,
	HEATING,
	COOLING,
	NUM_STATES
};

#define TC_STATE_MASK 0x7;	// 3 bits

#ifndef TEMP_CONTROL_STATIC
#define TEMP_CONTROL_STATIC 0
#endif

#if TEMP_CONTROL_STATIC
#define TEMP_CONTROL_METHOD static
#define TEMP_CONTROL_FIELD static
#else
#define TEMP_CONTROL_METHOD 
#define TEMP_CONTROL_FIELD
#endif

// Making all functions and variables static reduces code size.
// There will only be one TempControl object, so it makes sense that they are static.

/*
 * MDM: To support multi-chamber, I could have made TempControl non-static, and had a reference to
 * the current instance. But this means each lookup of a field must be done indirectly, which adds to the code size.
 * Instead, we swap in/out the sensors and control data so that the bulk of the code can work against compile-time resolvable
 * memory references. While the design goes against the grain of typical OO practices, the reduction in code size make it worth it.
 */

class TempControl{
	public:
	
	TempControl(){};
	~TempControl(){};
	
	TEMP_CONTROL_METHOD void init(void);
	TEMP_CONTROL_METHOD void reset(void);
	
	TEMP_CONTROL_METHOD void updateTemperatures(void);
	TEMP_CONTROL_METHOD void updatePID(void);
	TEMP_CONTROL_METHOD void updateState(void);
	TEMP_CONTROL_METHOD void updateOutputs(void);
	TEMP_CONTROL_METHOD void detectPeaks(void);
	
	TEMP_CONTROL_METHOD void loadSettings(eptr_t offset);
	TEMP_CONTROL_METHOD void storeSettings(eptr_t offset);
	TEMP_CONTROL_METHOD void loadDefaultSettings(void);
	
	TEMP_CONTROL_METHOD void loadConstants(eptr_t offset);
	TEMP_CONTROL_METHOD void storeConstants(eptr_t offset);
	TEMP_CONTROL_METHOD void loadDefaultConstants(void);
	
	//TEMP_CONTROL_METHOD void loadSettingsAndConstants(void);
		
	TEMP_CONTROL_METHOD uint16_t timeSinceCooling(void);
 	TEMP_CONTROL_METHOD uint16_t timeSinceHeating(void);
  	TEMP_CONTROL_METHOD uint16_t timeSinceIdle(void);
	  
	TEMP_CONTROL_METHOD fixed7_9 getBeerTemp(void);
	TEMP_CONTROL_METHOD fixed7_9 getBeerSetting(void);
	TEMP_CONTROL_METHOD void setBeerTemp(int newTemp);
	
	TEMP_CONTROL_METHOD fixed7_9 getFridgeTemp(void);
	TEMP_CONTROL_METHOD fixed7_9 getFridgeSetting(void);
	TEMP_CONTROL_METHOD void setFridgeTemp(int newTemp);
	
	TEMP_CONTROL_METHOD fixed7_9 getRoomTemp(void) {
		return ambientSensor->read();
	}
		
	TEMP_CONTROL_METHOD void setMode(char newMode);
	TEMP_CONTROL_METHOD char getMode(void) {
		return cs.mode;
	}

	TEMP_CONTROL_METHOD unsigned char getState(void){
		return state;
	}
		
	private:
	TEMP_CONTROL_METHOD void increaseEstimator(fixed7_9 * estimator, fixed7_9 error);
	TEMP_CONTROL_METHOD void decreaseEstimator(fixed7_9 * estimator, fixed7_9 error);
	TEMP_CONTROL_METHOD void constantsChanged();
	
	TEMP_CONTROL_METHOD void updateEstimatedPeak(uint16_t estimate, fixed7_9 estimator, uint16_t sinceIdle);

	public:
	TEMP_CONTROL_FIELD TempSensor* beerSensor;
	TEMP_CONTROL_FIELD TempSensor* fridgeSensor;
	TEMP_CONTROL_FIELD BasicTempSensor* ambientSensor;
	TEMP_CONTROL_FIELD Actuator* heater;
	TEMP_CONTROL_FIELD Actuator* cooler; 
	TEMP_CONTROL_FIELD Actuator* light;
	TEMP_CONTROL_FIELD Actuator* fan;
	TEMP_CONTROL_FIELD AutoOffActuator cameraLight;
	TEMP_CONTROL_FIELD Sensor<bool>* door;
	
	// Control parameters
	TEMP_CONTROL_FIELD ControlConstants cc;
	TEMP_CONTROL_FIELD ControlSettings cs;
	TEMP_CONTROL_FIELD ControlVariables cv;
		
	private:
	// keep track of beer setting stored in EEPROM
	TEMP_CONTROL_FIELD fixed7_9 storedBeerSetting;

	// Timers
	TEMP_CONTROL_FIELD unsigned int lastIdleTime;
	TEMP_CONTROL_FIELD unsigned int lastHeatTime;
	TEMP_CONTROL_FIELD unsigned int lastCoolTime;
	
	// State variables
	TEMP_CONTROL_FIELD uint8_t state;
	TEMP_CONTROL_FIELD bool doPosPeakDetect;
	TEMP_CONTROL_FIELD bool doNegPeakDetect;
	
	friend class TempControlState;
};
	
extern TempControl tempControl;


#endif /* CONTROLLER_H_ */