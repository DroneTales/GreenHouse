#include "Pins.h"
#include "Sensors.h"

#include <OneWireESP32.h>


/******************************************************************************/
/*                       Temperature sensors  constants                       */

// Delay after powering up sensors.
constexpr uint32_t SENSORS_INIT_DELAY = 1000;
// Delay before temperature reading.
constexpr uint32_t SENSORS_READING_DELAY = 750;

/******************************************************************************/


/******************************************************************************/
/*                       Temperature sensors  functions                       */

// Initializes the temperature sensors hardware.
void SensorsInit()
{
    // Initialize the temperature sensors pins. We must do it to remove
    // power from the sensors.
    pinMode(DS18B20_POWER_PIN, OUTPUT);    
    // Disable the sensors power.
    digitalWrite(DS18B20_POWER_PIN, LOW);
}

// Reads temperature values from the temperature sensors.
void SensorsGetTemperature(SENSORS_CONFIG& Config, SENSORS_DATA& Data)
{
    // Turns sensor on.
    digitalWrite(DS18B20_POWER_PIN, HIGH);
    // Wait for sensors initialization.
    delay(SENSORS_INIT_DELAY);
    
    // Initializa DS18B20 sensor.
    OneWire32 Sensor(DS18B20_PIN);
    
    // Request temperature from all connected sensros.
    Sensor.request();
    delay(SENSORS_READING_DELAY);
    
    // Read temperature.
    Data.Count = Config.Count;
    for (uint8_t i = 0; i < Config.Count; i++)
    {
        // getTemp return 0 if success!
        if (Sensor.getTemp(Config.Address[i], Data.Sensors[i]))
            Data.Sensors[i] = NAN;
    }
    
    // Turns sensor off.
    digitalWrite(DS18B20_POWER_PIN, LOW);
}

/******************************************************************************/
