#pragma once

#include <Arduino.h>


/******************************************************************************/
/*                        On-board battery data  types                        */

// The structure contains the on-board battery information.
struct BATTERY_INFO
{
    // Current raw battery voltage. Used for loggin only.
    float       Voltage;
    // The current ob-board battery capacity.
    uint8_t     Capacity;
};

/******************************************************************************/


/******************************************************************************/
/*                         On-board battery functions                         */

// Initializes the on-board battery pins.
void BattInit();
// Enables the on-board battery power source.
void BattEnable();
// Reads the on-board battery information.
void BattGetInfo(BATTERY_INFO& Info);
// Disables the on-board battery power source.
void BattDisable();

/******************************************************************************/
