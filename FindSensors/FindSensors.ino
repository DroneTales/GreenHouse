#include "OneWireESP32.h"

// Comment this line if you do not use separate power pin for power up
// DS18B20.
#define _USE_POWER_PIN_

// Power enable pin.
#define SENSOR_POWER_PIN	GPIO_NUM_18
// Sensor DQ pin.
#define SENSOR_DATA_PIN		GPIO_NUM_19

const uint8_t MaxDevs = 10;
float currTemp[MaxDevs];

void setup()
{
	#ifdef _USE_POWER_PIN_
		// Power on sensors.
		pinMode(SENSOR_POWER_PIN, OUTPUT);
		digitalWrite(SENSOR_POWER_PIN, HIGH);
	#endif

	OneWire32 ds(SENSOR_DATA_PIN);
	
	delay(1000);
	Serial.begin(115200);
	
	uint64_t addr[MaxDevs];
	uint8_t devices = ds.search(addr, MaxDevs);
	for (uint8_t i = 0; i < devices; i += 1)
		Serial.printf("%d: 0x%llx [%llu]\n", i, addr[i], addr[i]);

	ds.request();
	delay(750);
	for (uint8_t i = 0; i < MaxDevs; i++)
	{
		uint8_t err = ds.getTemp(addr[i], currTemp[i]);
		if (err)
		{
			const char *errt[] = {"", "CRC", "BAD","DC","DRV"};
			Serial.print(i); Serial.print(": "); Serial.println(errt[err]);
		}
		else
			Serial.print(i); Serial.print(": "); Serial.println(currTemp[i]);
	}
}


void loop()
{
}
