// LilyGO T-A7608E-H board
// ===============================================================
// Arduino IDE settings:
//   - Board: ESP32 WROVER Kit (all versions)
//   - CPU Frequency: 80MHz (WiFi)
//   - Core Debug Level: None
//   - Erase All Flash Before Sketch Upload: Disabled
//   - Flash frequency: 80Mhz
//   - Flash Mode: QIO
//   - Flash Size: 4MB (32Mb)
//   - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
//   - PSRAM: Enabled
//   - Upload Speed: 921600
// ===============================================================

#include "Config.h"
#include "Defines.h"
#include "Pins.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_wifi.h>

#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER      1024

#include <OneWireESP32.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>


/*******************************************************************************/
/*                                    UARTs                                    */

// Debug UART configuration.
#define DebugSerial             Serial
#define DEBUG_UART_BAUD_RATE    115200

// Modem UART configuration.
#define ModemSerial             Serial1
#define MODEM_UART_BAUD_RATE    115200


/*******************************************************************************/
/*                            Tags for debug output                            */

const char* const TAG_BATTERY = "BATTERY";
const char* const TAG_POWER = "POWER";
const char* const TAG_SENSOR = "SENSOR";
const char* const TAG_MODEM = "MODEM";
const char* const TAG_GPRS = "GPRS";
const char* const TAG_MQTT = "MQTT";


/*******************************************************************************/
/*                                 Data  types                                 */

// Onboard battery data.
struct BATTERY_DATA
{
    // Read voltage.
    float   Voltage;
    // Adjusted voltage.
    float   AdjustedVoltage;
    // Capacity in percents.
    uint8_t Capacity;
};

// Temperature data.
struct TEMPERATURE_DATA
{
    float   AvgTemperature;
    float   Sensors[SENSOR_ZONES_COUNT];
};


/*******************************************************************************/
/*                               Global  objects                               */

// Modem object.
TinyGsm Modem(ModemSerial);
// GPRS client object.
TinyGsmClient Client(Modem);
// MQTT client object.
PubSubClient MqttClient(Client);


/*******************************************************************************/
/*                          Onboard battery  routines                          */

void InitBattery()
{
    // Initialize onboard battery pins.
    pinMode(BATT_18650_EN_PIN, OUTPUT);
    pinMode(BATT_18650_ADC_PIN, INPUT);
    // Enable onboard battery.
    digitalWrite(BATT_18650_EN_PIN, HIGH);
}

void ReadBatteryData(BATTERY_DATA& Data)
{
    // Read battery voltage. Following by the board schematics there is 1:2 voltage
    // devider on the battery ADC pin.
    Data.Voltage = (float)analogReadMilliVolts(BATT_18650_ADC_PIN) / 1000.0 *
        BATTERY_VOLTAGE_DIVIDER;

    // Adjust voltage.
    Data.AdjustedVoltage = Data.Voltage;
    if (Data.AdjustedVoltage < EMPTY_BATTERY_VOLTS)
        Data.AdjustedVoltage = EMPTY_BATTERY_VOLTS;
    if (Data.AdjustedVoltage > FULL_BATTERY_VOLTS)
        Data.AdjustedVoltage = FULL_BATTERY_VOLTS;

    // Calculate battery capacity
    Data.Capacity = (uint8_t)(100.0 * ((Data.AdjustedVoltage - EMPTY_BATTERY_VOLTS) /
        (FULL_BATTERY_VOLTS - EMPTY_BATTERY_VOLTS)));
}


/*******************************************************************************/
/*                        Temperature sensors rountines                        */

void ReadTemperature(TEMPERATURE_DATA& Data)
{
    // Turns sensor on.
    digitalWrite(DS18B20_POWER_PIN, HIGH);
    delay(200);

    // Initializa DS18B20 sensor.
    OneWire32 Sensor(DS18B20_PIN);

    // Request temperature from all connected sensros.
    Sensor.request();
    delay(SENSOR_READING_DELAY);

    // Read temperature.
    Data.AvgTemperature = 0.0;
    uint8_t TotalSensors = 0;
    for (uint8_t i = 0; i < SENSOR_ZONES_COUNT; i++)
    {
        if (Sensor.getTemp(SENSORS[i], Data.Sensors[i]))
            Data.Sensors[i] = NAN;
        else
        {
            Data.AvgTemperature += Data.Sensors[i];
            TotalSensors++;
        }
    }

    // Turns sensor off.
    digitalWrite(DS18B20_POWER_PIN, LOW);

    // Calculate temperature.
    if (TotalSensors == 0)
        Data.AvgTemperature = NAN;
    else
        Data.AvgTemperature = Data.AvgTemperature / (float)TotalSensors;
}


/*******************************************************************************/
/*                               Modem rountines                               */

bool IsModemActive()
{
    // Clear modem stream.
    while (Modem.stream.available())
        Modem.stream.read();

    for (uint8_t i = 0; i < 2; i++)
    {
        if (Modem.testAT(AT_TIMEOUT))
            return true;
        delay(MODEM_TEST_DELAY);
    }

    return false;
}

void InitModemHardware()
{
    // Initialize modem control pins.
    pinMode(A7608H_DTR_PIN, OUTPUT);
    pinMode(A7608H_RESET_PIN, OUTPUT);
    pinMode(A7608H_PWRKEY_PIN, OUTPUT);
    // Set modem pins to initial state.
    digitalWrite(A7608H_DTR_PIN, LOW);
    digitalWrite(A7608H_RESET_PIN, LOW);
    digitalWrite(A7608H_PWRKEY_PIN, LOW);
    // Initialize modem UART.
    ModemSerial.begin(MODEM_UART_BAUD_RATE, SERIAL_8N1,
        A7608H_RX_PIN, A7608H_TX_PIN);

    // Modem can be turned on after reset. Wait for initialization.
    delay(MODEM_WAKEUP_DELAY);
}

bool InitModem()
{
    // Check modem state.
    if (!IsModemActive())
    {
        digitalWrite(A7608H_PWRKEY_PIN, HIGH);
        delay(POWER_ON_PULSE);

        digitalWrite(A7608H_PWRKEY_PIN, LOW);
        delay(POWER_ON_DELAY);
    }

    // Wait for modem initialization.
    uint8_t Retry = 0;
    bool ModemReady = false;
    while (!ModemReady && Retry++ < INIT_RETRY)
        ModemReady = IsModemActive();
    return ModemReady;
}

bool InitSim()
{
    // Check SIM state. Enter PIN if needed.
    switch (Modem.getSimStatus())
    {
        case SIM_ERROR:
            return false;

        case SIM_LOCKED:
            if (Modem.simUnlock("1234"))
                return false;
            break;

        case SIM_ANTITHEFT_LOCKED:
            return false;
    }
    delay(SIM_READ_DELAY);

    return (Modem.getSimStatus() == SIM_READY);
}

void UninitModem()
{
    // Try to turn modem off with AT command (software power off).
    Modem.sendAT("+CPOF");
    if (Modem.waitResponse(AT_TIMEOUT))
        return;

    // If failed use power pins (hardware power off).
    // First make sure modem is alive. Otherwise we can turn it ON
    // instead.
    if (!IsModemActive())
        return;

    digitalWrite(A7608H_PWRKEY_PIN, HIGH);
    delay(POWER_OFF_PULSE);

    digitalWrite(A7608H_PWRKEY_PIN, LOW);
    delay(POWER_OFF_DELAY);
}


/*******************************************************************************/
/*                           Mobile network routines                           */

bool ConnectToNetwork()
{
    // Wait for network connection.
    uint8_t Retry = 0;
    while (Retry++ < NETWORK_CHECK_RETRY && !Modem.isNetworkConnected())
        delay(NETWORK_CHECK_INTERVAL);
    return Modem.isNetworkConnected();
}

bool ConnectToGprs()
{
    // Initialize SIM card.
    if (!InitSim())
        return false;
    // Connect to mobile network.
    if (!ConnectToNetwork())
        return false;

    // Check GPRS connection.
    if (Modem.isGprsConnected())
        return true;

    // Connect to GPRS.
    uint8_t Retry = 0;
    while (Retry < GPRS_CONNECT_RETRY)
    {
        if (Modem.gprsConnect(GPRS_APN, GPRS_USER_NAME, GPRS_PASSWORD))
        {
            delay(GPRS_INIT_DELAY);

            uint8_t TestRetry = 0;
            while (TestRetry < GPRS_TEST_RETRY)
            {
                if (Modem.isGprsConnected())
                    return true;

                TestRetry++;
                delay(GPRS_TEST_DELAY);
            }
            Modem.gprsDisconnect();
        }
        Retry++;
        delay(GPRS_CONNECT_DELAY);
    }

    return false;
}

void DisconnectFromGprs()
{
    Modem.gprsDisconnect();
}


/*******************************************************************************/
/*                                MQTT routines                                */

bool ConnectToMqtt()
{
    // Check MQTT connection status. It should not be connected
    // but to be sure.
    if (MqttClient.connected())
        return true;

    // Set MQTT server parameters.
    MqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    MqttClient.setSocketTimeout(MQTT_TIMEOUT);

    // Try to connect to MQTT broker.
    uint8_t Retry = 0;
    while (Retry < MQTT_CONNECT_RETRY)
    {
        if (MqttClient.connect(MQTT_CLIENT_ID, MQTT_USER_NAME, MQTT_PASSWORD))
            break;
        
        Retry++;
        delay(MQTT_CONNECT_DELAY);
    }

    // Check MQTT connection state.
    return (MqttClient.state() == MQTT_CONNECTED);
}

void DisconnectFromMqtt()
{
    MqttClient.flush();
    MqttClient.disconnect();
}

bool PublishBatteryData(const BATTERY_DATA& BatteryData)
{
    char Message[30] = { 0 };
    sprintf(Message, "%u", BatteryData.Capacity);
    // The battery capacity is important data. If we are not able to
    // publish it we must re-try in short period of time.
    if (!MqttClient.publish(MQTT_TOPIC_BATTERY_CAPACITY, Message))
        return false;

    // This is not important data so it does not matter if we publish or not
    // any that.
    memset(Message, 0, 30);
    sprintf(Message, "%s",
        BatteryData.Capacity <= LOW_BATTERY_CAPACITY ? "true" : "false");
    MqttClient.publish(MQTT_TOPIC_BATTERY_LOW, Message);

    memset(Message, 0, 30);
    sprintf(Message, "%f", BatteryData.Voltage);
    MqttClient.publish(MQTT_TOPIC_BATTERY_VOLTAGE, Message);

    memset(Message, 0, 30);
    sprintf(Message, "%f", BatteryData.AdjustedVoltage);
    MqttClient.publish(MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE, Message);

    return true;
}

bool PublishTemperatureData(const TEMPERATURE_DATA& TemperatureData)
{
    char Message[30] = { 0 };
    sprintf(Message, "%f",
        isnan(TemperatureData.AvgTemperature) ? SENSOR_INVALID_VALUE : TemperatureData.AvgTemperature);
    // The averrage temperature is important data. So if we were not able to publish it
    // we must re-try in short period of time.
    if (!MqttClient.publish(MQTT_TOPIC_TEMPERATURE_AVG, Message))
        return false;

    // The individual sensors data is not important so we do not check publishing result.
    for (uint8_t i = 0; i < SENSOR_ZONES_COUNT; i++)
    {
        char Topic[80] = { 0 };
        sprintf(Topic, MQTT_TOPIC_TEMPERATURE_SENSOR, i);
        memset(Message, 0, 30);
        sprintf(Message, "%f",
            isnan(TemperatureData.Sensors[i]) ? SENSOR_INVALID_VALUE : TemperatureData.Sensors[i]);
        MqttClient.publish(Topic, Message);
    }

    return true;
}


/*******************************************************************************/
/*                               Data processing                               */

bool ProcessData()
{
    // Read onboard battery data.
    BATTERY_DATA BatteryData = { 0 };
    ReadBatteryData(BatteryData);
    // Try to publish battery data.
    if (!PublishBatteryData(BatteryData))
        return false;

    // Read temperature sensors data.
    TEMPERATURE_DATA TemperatureData = { 0 };
    ReadTemperature(TemperatureData);
    // Sent data to MQTT broker.
    if (!PublishTemperatureData(TemperatureData))
        return false;

    // The averrage temperature is important. So if it was not read
    // correctly we must re-try in short period of time.
    return !isnan(TemperatureData.AvgTemperature);
}


/*******************************************************************************/
/*                              Arduino  routines                              */

void DisableBluetoothAndWiFi()
{
    esp_bluedroid_disable();
    esp_bt_controller_disable();
    esp_wifi_stop();
}

void Hibernate(const bool Success)
{
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);
    //esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

    // Set wakeup timer.
    esp_sleep_enable_timer_wakeup(Success ? SLEEP_DURATION_SUCCESS : SLEEP_DURATION_FAILED);
    // Start sleep.
    esp_deep_sleep_start();
}

void setup()
{
    // Disable brown-out
    uint32_t BownOutState = READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // Initialize onboard battery.
    InitBattery();

    // Disable Bluetooth and Wi-Fi as we do not use it.
    DisableBluetoothAndWiFi();

    // Init DS18B20 power pin.
    pinMode(DS18B20_POWER_PIN, OUTPUT);
    digitalWrite(DS18B20_POWER_PIN, LOW);

    // Initialize debug UART.
    DebugSerial.begin(DEBUG_UART_BAUD_RATE);

    // Initialize modem hardware.
    InitModemHardware();

    // Data collecation and publishing result. Assume it is failed.
    bool Success = false;
    // Initialize modem.
    if (InitModem())
    {
        // Connect to Internet.
        if (ConnectToGprs())
        {
            // Connect to MQTT broker.
            if (ConnectToMqtt())
            {
                // Collect and send data.
                Success = ProcessData();
                // Disconnect from MQTT broker.
                DisconnectFromMqtt();
            }
            // Disconnect from Internet.
            DisconnectFromGprs();
        }
        // Turn modem off.
        UninitModem();
    }
    // Restore brown out detection state.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, BownOutState);
    // Put board in sleep.
    Hibernate(Success);
}

void loop()
{
}