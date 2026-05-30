#include "Modem.h"
#include "Mqtt.h"


/******************************************************************************/
/*                               MQTT constants                               */

// MQTT connection retry delay.
constexpr uint32_t MQTT_RECONNECT_DELAY = 1000;
// MQTT connection attempts number.
constexpr uint8_t MQTT_CONNECT_RETRY = 10;

// MQTT battery capacity topics.
const char* const MQTT_TOPIC_BATTERY_CAPACITY = "greenhouse/battery";
// MQTT battery voltage topic. Used only for logging.
const char* const MQTT_TOPIC_BATTERY_VOLTAGE = "greenhouse/battery/voltage";
// MQTT sensoes termperature topics.
const char* const MQTT_TOPIC_SENSORS = "greenhouse/sensors/%u";

/******************************************************************************/


/******************************************************************************/
/*                               MQTT functions                               */

// Connects to MQTT broker. Returns true if connection was success. Returns
// false otherwise.
bool MqttConnect(const MQTT_CONFIG& Config)
{
    // Check MQTT connection status. It should not be connected but to
    // be sure.
    if (MqttClient.connected())
        return true;
    
    // Set MQTT server parameters.
    MqttClient.setServer(Config.Server, Config.Port);
    
    // Try to connect to MQTT broker.
    uint8_t Retry = 0;
    for (uint8_t Retry = 0; Retry < MQTT_CONNECT_RETRY; Retry++)
    {
        if (MqttClient.connect(Config.ClientId, Config.UserName, Config.Password))
            break;
        
        delay(MQTT_RECONNECT_DELAY);
    }
    return MqttClient.connected();
}

// Publishes the sensors data. Returns true if publication was success.
// Returns false otherwise.
bool MqttPublish(const BATTERY_INFO& BatteryInfo,
    const SENSORS_DATA& SensorsData)
{
    char Message[30] = { 0 };
    char Topic[80] = { 0 };
    
    // Publish the battery capacity. It is critical information so
    // we can not ignore the result.
    sprintf(Message, "%u", BatteryInfo.Capacity);
    if (!MqttClient.publish(MQTT_TOPIC_BATTERY_CAPACITY, Message))
        return false;

    // Publish battery voltage. As it is used for logging only we
    // can simple ignore publishing error if any.
    memset(Message, 0, sizeof(Message));
    sprintf(Message, "%.2f", BatteryInfo.Voltage);
    MqttClient.publish(MQTT_TOPIC_BATTERY_VOLTAGE, Message);
    
    // Publish sensors temperature.
    for (uint8_t i = 0; i < SensorsData.Count; i++)
    {
        memset(Message, 0, sizeof(Message));
        memset(Topic, 0, sizeof(Topic));

        sprintf(Topic, MQTT_TOPIC_SENSORS, i);
        sprintf(Message, "%.1f", SensorsData.Sensors[i]);
        if (!MqttClient.publish(Topic, Message))
            return false;
    }
    
    return true;
}

// Disconnects from MQTT broker.
void MqttDisconnect()
{
    if (MqttClient.connected())
    {
        MqttClient.flush();
        MqttClient.disconnect();
    }
}

/******************************************************************************/
