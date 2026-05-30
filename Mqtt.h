#pragma once

#include "Batt.h"
#include "Config.h"
#include "Sensors.h"


/******************************************************************************/
/*                               MQTT functions                               */

// Connects to MQTT broker. Returns true if connection was success. Returns
// false otherwise.
bool MqttConnect(const MQTT_CONFIG& Config);
// Publishes the sensors data.
bool MqttPublish(const BATTERY_INFO& BatteryInfo,
    const SENSORS_DATA& SensorsData);
// Disconnects from MQTT broker.
void MqttDisconnect();

/******************************************************************************/
