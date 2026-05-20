#include "Modem.h"
#include "Pins.h"


/******************************************************************************/
/*                          On-board modem constants                          */

// Define modem UART port for easy use in code.
#define ModemSerial                 Serial1

// Modem UART baud rate.
constexpr unsigned long MODEM_UART_BAUD_RATE = 115200;

// Modem power on timings.
constexpr uint32_t MODEM_POWER_ON_PULSE = 1000;
constexpr uint32_t MODEM_POWER_ON_DELAY = 5000;

// Modem power off timings.
constexpr uint32_t MODEM_POWER_OFF_PULSE = 5000;
constexpr uint32_t MODEM_POWER_OFF_DELAY = 7000;

// Modem initialization re-try counter.
constexpr uint8_t MODEM_INIT_RETRY = 10;

// AT command timeout.
constexpr uint32_t MODEM_AT_TIMEOUT = 1000;
// Modem test delay.
constexpr uint32_t MODEM_TEST_DELAY = 100;

// Modem SIM ready delay.
constexpr uint32_t MODEM_SIM_READY_DELAY = 200;

// How many times we will check for network.
constexpr uint8_t MODEM_NETWORK_TEST_RETRY = 60;
// Network ready check interval.
constexpr uint32_t MODEM_NETWORK_TEST_DELAY = 1000;

// GPRS connection attempts.
constexpr uint8_t MODEM_GPRS_CONNECT_RETRY = 3;
// GPRS initialization delay.
constexpr uint32_t MODEM_GPRS_INIT_DELAY = 5000;
// GPRS test attempts.
constexpr uint8_t MODEM_GPRS_TEST_RETRY = 3;
// GPRS test delay.
constexpr uint32_t MODEM_GPRS_TEST_DELAY = 5000;
// Delay before GPRS connect.
constexpr uint32_t MODEM_GPRS_CONNECT_DELAY = 5000;

/******************************************************************************/


/******************************************************************************/
/*                               Global objects                               */

// Modem object.
TinyGsm Modem(ModemSerial);
// GPRS client object.
TinyGsmClient Client(Modem);
// MQTT client object.
PubSubClient MqttClient(Client);

/******************************************************************************/


/******************************************************************************/
/*                      On-board modem helper  functions                      */

// Tests modem availability. Returns true if the modem up and running.
// Returns false otherwise.
bool IsModemReady()
{
    // Clear modem stream.
    while (Modem.stream.available())
        Modem.stream.read();
    
    // Try to execute AT command.
    for (uint8_t i = 0; i < 2; i++)
    {
        if (Modem.testAT(MODEM_AT_TIMEOUT))
            return true;
        delay(MODEM_TEST_DELAY);
    }

    return false;
}

// Initializes the SIM card. Return true if SIM was initialized. Returns false
// otherwise.
bool InitSim(const char* const Pin)
{
    // Initialize the SIM card before connectiong to a mobile network.
    int Status = Modem.getSimStatus();
    // SIM locked by PIN?
    if (Status == SIM_LOCKED)
    {
        // Try to unlock by providing the PIN.
        Modem.simUnlock(Pin);
        // Wait for SIM initialization.
        delay(MODEM_SIM_READY_DELAY);
        // Read SIM status once again.
        Status = Modem.getSimStatus();
    }
    // SIM ready?
    return (Status == SIM_READY);
}

/******************************************************************************/


/******************************************************************************/
/*                          On-board modem functions                          */

// Initializes the on-board modem unit.
void ModemInit()
{
    // Initialize the modem pins and set all of them to LOW state so modem
    // can not be turned on randomly.
    pinMode(A7608H_DTR_PIN, OUTPUT);
    pinMode(A7608H_RESET_PIN, OUTPUT);
    pinMode(A7608H_PWRKEY_PIN, OUTPUT);

    // Set the modem pins to LOW state. It prevents modem from
    // auto-start when we enable its power delivery from the on-board
    // battery.
    digitalWrite(A7608H_DTR_PIN, LOW);
    digitalWrite(A7608H_RESET_PIN, LOW);
    digitalWrite(A7608H_PWRKEY_PIN, LOW);

    // Initialize the modem UART.
    ModemSerial.begin(MODEM_UART_BAUD_RATE, SERIAL_8N1, A7608H_RX_PIN,
        A7608H_TX_PIN);
}

// Turns the on-board modem unit on. Returns true if the modem unit is up
// and running. Returns false otherwise.
bool ModemTurnOn()
{
    // Turn on the modem power by sending power on pulse
    // ("press" the power button).
    digitalWrite(A7608H_PWRKEY_PIN, HIGH);
    delay(MODEM_POWER_ON_PULSE);
    digitalWrite(A7608H_PWRKEY_PIN, LOW);
    
    // Wait for initialization.
    delay(MODEM_POWER_ON_DELAY);
    for (uint8_t Retry = 0; Retry < MODEM_INIT_RETRY; Retry++)
    {
        if (IsModemReady())
            return true;
    }
    
    return false;
}

// Connects to a mobile network. Returns true if a mobile network is
// available. Returns false otherwise.
bool ModemConnectToNetwork(const char* const Pin)
{
    // Before connecting to a network we must initialize SIM card.
    if (!InitSim(Pin))
        return false;

    // Wait for mobile network.
    for (uint8_t Retry = 0; Retry < MODEM_NETWORK_TEST_RETRY; Retry++)
    {
        if (Modem.isNetworkConnected())
            return true;
        delay(MODEM_NETWORK_TEST_DELAY);
    }
    return false;
}

// Connects to GPRS. Return true if success. Returns false otherwise.
bool ModemConnectToGprs(const GPRS_CONFIG& Config)
{
    // Check the GPRS connection status. We do notneed to re-connect
    // if it is already connected.
    if (Modem.isGprsConnected())
        return true;

    // Now try to connect to GPRS service.
    for (uint8_t Retry = 0; Retry < MODEM_GPRS_CONNECT_RETRY; Retry++)
    {
        if (Modem.gprsConnect(Config.Apn, Config.UserName, Config.Password))
        {
            delay(MODEM_GPRS_INIT_DELAY);
            for (uint8_t TestRetry = 0; TestRetry < MODEM_GPRS_TEST_RETRY; TestRetry++)
            {
                if (Modem.isGprsConnected())
                    return true;
                
                delay(MODEM_GPRS_TEST_DELAY);
            }
            Modem.gprsDisconnect();
        }
        delay(MODEM_GPRS_CONNECT_DELAY);
    }
    return false;
}

// Disconnects from GPRS service.
void ModemDisconnectFromGprs()
{
    // Make sure GPRS is still connected.
    if (Modem.isGprsConnected())
        Modem.gprsDisconnect();
}

// Turns the on-board modem unit off.
void ModemTurnOff()
{
    // We must check that modem is still up and running.
    // Otherwise we can turn it on instead of turning it off.
    if (!IsModemReady())
        return;
    
    // Try to turn modem off with AT command
    // (software power off).
    Modem.sendAT("+CPOF");
    if (Modem.waitResponse(MODEM_AT_TIMEOUT))
        return;
    
    // If failed then use hardware pins to force power off.
    digitalWrite(A7608H_PWRKEY_PIN, HIGH);
    delay(MODEM_POWER_OFF_PULSE);
    digitalWrite(A7608H_PWRKEY_PIN, LOW);

    // Wait until modem unit is powered off.
    delay(MODEM_POWER_OFF_DELAY);
}

/******************************************************************************/
