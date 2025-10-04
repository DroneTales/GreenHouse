#pragma once

/*******************************************************************************/
/*                         Temperature sensor settings                         */

// Number of zones.
#define SENSOR_ZONES_COUNT			    4

// Zone 1 sensor addresses.
#define GREEN_ADDRESS               9666714504207294760ul
// Zone 2 sensor addresses.
#define YELLOW_ADDRESS              18437195502557880616ul
// Zone 3 sensor addresses.
#define BLACK_ADDRESS               290801794589212200ul
// Zone 4 sensor addresses.
#define RED_ADDRESS                 16791709353807948072ul

static uint64_t SENSORS[SENSOR_ZONES_COUNT] =
{
  GREEN_ADDRESS,
  YELLOW_ADDRESS,
  BLACK_ADDRESS,
  RED_ADDRESS
};


/*******************************************************************************/
/*                          Onboard battery  settings                          */

// The empty battery voltage.
#define EMPTY_BATTERY_VOLTS         3.4
// The full battery voltage.
#define FULL_BATTERY_VOLTS          4.2
// The voltage divider. By schematic it is 1:2 but actual
// value may vary. In my case it is 1:1.95
#define BATTERY_VOLTAGE_DIVIDER		  1.95
// Low battery capacity.
#define LOW_BATTERY_CAPACITY        10


/*******************************************************************************/
/*                                GPRS settings                                */

#define GPRS_APN              	    "<_GPRS_APN_>"
#define GPRS_USER_NAME              "<_GPRS_USER_NAME_>"
#define GPRS_PASSWORD               "<_GPRS_PASSWORD_>"
#define GPRS_SIM_PIN                "<_SIM_PIN_>"

/*******************************************************************************/
/*                            MQTT  server settings                            */

#define MQTT_SERVER                 "<_MQTT_SERVER_ADDRESS_>"
#define MQTT_PORT                   <_MQTT_SERVER_PORT_>
#define MQTT_USER_NAME              "<_MQTT_USER_NAME_>"
#define MQTT_PASSWORD               "<_MQTT_PASSWORD_>"
#define MQTT_CLIENT_ID              "GREEN_HOUSE"

