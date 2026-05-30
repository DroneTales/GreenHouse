#pragma once

#include <Arduino.h>


/******************************************************************************/
/*                          Configuration  constants                          */

// GPRS APN maxumum length.
constexpr uint8_t GPRS_APN_LEN = 64;
// GPRS user name maximum length.
constexpr uint8_t GPRS_USER_NAME_LEN = 64;
// GPRS password maximum length.
constexpr uint8_t GPRS_PASSWORD_LEN = 64;
// SIM PIN maximum length.
constexpr uint8_t GPRS_SIM_PIN_LEN = 4;

// MQTT broker name maximum length.
constexpr uint8_t MQTT_SERVER_NAME_LEN = 128;
// MQTT broker user name maximum length.
constexpr uint8_t MQTT_USER_NAME_LEN = 64;
// MQTT broker password maximum length.
constexpr uint8_t MQTT_PASSWORD_LEN = 64;
// MQTT client ID maximum length.
constexpr uint8_t MQTT_CLIENT_ID_LEN = 64;

// Maximum possible number of connected sensors.
constexpr uint8_t SENSORS_MAX_NUMBER = 10;

/******************************************************************************/


/******************************************************************************/
/*                          Configuration data types                          */

// Board configuration.
struct BOARD_CONFIG
{
    // Short sleep interval in microseconds.
    uint64_t    ShortSleep;
    // Long sleep interval in milliseconds.
    uint64_t    LongSleep;
};

// GPRS configuration.
struct GPRS_CONFIG
{
    char    Apn[GPRS_APN_LEN + 1];
    char    UserName[GPRS_USER_NAME_LEN + 1];
    char    Password[GPRS_PASSWORD_LEN + 1];
    char    Pin[GPRS_SIM_PIN_LEN + 1];
};

// MQTT configuration.
struct MQTT_CONFIG
{
    // MQTT server (broker) address.
    char        Server[MQTT_SERVER_NAME_LEN + 1];
    uint16_t    Port;
    char        UserName[MQTT_USER_NAME_LEN + 1];
    char        Password[MQTT_PASSWORD_LEN + 1];
    char        ClientId[MQTT_CLIENT_ID_LEN + 1];
};

// Sensors configuration.
struct SENSORS_CONFIG
{
    // Number of sensors addresses in the address array.
    uint8_t     Count;
    uint64_t    Address[SENSORS_MAX_NUMBER];
};

// Full configuration.
struct CONFIG
{
    BOARD_CONFIG    Board;
    GPRS_CONFIG     Gprs;
    MQTT_CONFIG     Mqtt;
    SENSORS_CONFIG  Sensors;
};

/******************************************************************************/


/******************************************************************************/
/*                          Configuration  functions                          */

// Reads configuration from SD-card. Returns true if the configuration is
// valid. Returns false otherwise.
bool ConfigRead(CONFIG& Config);

/******************************************************************************/
