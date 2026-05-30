#pragma once

#include "Config.h"


/******************************************************************************/
/*                       Temperature sensors data types                       */

// Temperature data.
struct SENSORS_DATA
{
    // Number of sensors in sesnors array.
    uint8_t     Count;
    float       Sensors[SENSORS_MAX_NUMBER];
};

/******************************************************************************/


/******************************************************************************/
/*                       Temperature sensors  functions                       */

// Initializes the temperature sensors hardware.
void SensorsInit();
// Reads temperature values from the temperature sensors.
void SensorsGetTemperature(SENSORS_CONFIG& Config, SENSORS_DATA& Data);

/******************************************************************************/
