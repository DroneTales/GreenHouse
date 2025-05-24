#pragma once

/*******************************************************************************/
/*                               Sleep  settings                               */

// Sleep duration when data was read and send with success.
#define SLEEP_DURATION_SUCCESS              (15 * 60 * 1000000)
// Sleep duration when data was not read or was not send.
#define SLEEP_DURATION_FAILED               (2 * 60 * 1000000)


/*******************************************************************************/
/*                      Temperature sensor  configuration                      */

// Delay before temperature reading.
#define SENSOR_READING_DELAY                750
// Used as invalid temperature value when sensor not responding.
#define SENSOR_INVALID_VALUE                (-100.0)


/*******************************************************************************/
/*                                MQTT settings                                */

// Connection configuration.
#define MQTT_TIMEOUT                        20
#define MQTT_CONNECT_DELAY                  5000
#define MQTT_CONNECT_RETRY                  23

// Temperature sensors topics.
#define MQTT_TOPIC_TEMPERATURE_AVG          "greenhouse/temperature"
#define MQTT_TOPIC_TEMPERATURE_SENSOR       "greenhouse/sensors/%u"

// Battery topics.
#define MQTT_TOPIC_BATTERY_CAPACITY         "greenhouse/battery"
#define MQTT_TOPIC_BATTERY_LOW              "greenhouse/battery/low"
#define MQTT_TOPIC_BATTERY_VOLTAGE          "greenhouse/voltage/voltage"
#define MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE  "greenhouse/voltage/adjusted"


/*******************************************************************************/
/*                             Modem configuration                             */

// Modem power timings.
#define MODEM_WAKEUP_DELAY                  10000

#define POWER_ON_PULSE                      1000
#define POWER_ON_DELAY                      5000

#define POWER_OFF_PULSE                     5000
#define POWER_OFF_DELAY                     7000

// Modem initialization re-try counter.
#define INIT_RETRY                          10

// Modem test delay.
#define MODEM_TEST_DELAY                    100

// AT command timeout.
#define AT_TIMEOUT                          1000

// Modem IMEI read delay.
#define IMEI_READ_DELAY                     100
// Modem SIM delays.
#define SIM_INIT_DELAY                      5000
// Modem SIM ready delay.
#define SIM_READ_DELAY                      200

// Network ready check interval.
#define NETWORK_CHECK_INTERVAL              1000
// How many times we will check for network.
#define NETWORK_CHECK_RETRY                 60

// GPRS initialization delay.
#define GPRS_INIT_DELAY                     5000

// GPRS connection attempts.
#define GPRS_CONNECT_RETRY                  3
#define GPRS_CONNECT_DELAY                  5000

// GPRS test attempts.
#define GPRS_TEST_RETRY                     3
#define GPRS_TEST_DELAY                     5000
