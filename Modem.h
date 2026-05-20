#pragma once

#include "Config.h"

// TinyGsmClient library configuration.
#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER			1024

#include <TinyGsmClient.h>
#include <PubSubClient.h>


/******************************************************************************/
/*                               Global objects                               */

// MQTT client object.
extern PubSubClient MqttClient;

/******************************************************************************/


/******************************************************************************/
/*                          On-board modem functions                          */

// Initializes the on-board modem unit.
void ModemInit();
// Turns the on-board modem unit on. Returns true if the modem unit is up
// and running. Returns false otherwise.
bool ModemTurnOn();
// Connects to a mobile network. Returns true if a mobile network is
// available. Returns false otherwise.
bool ModemConnectToNetwork(const char* const Pin);
// Connects to GPRS. Return true if success. Returns false otherwise.
bool ModemConnectToGprs(const GPRS_CONFIG& Config);
// Disconnects from GPRS service.
void ModemDisconnectFromGprs();
// Turns the on-board modem unit off.
void ModemTurnOff();

/******************************************************************************/
