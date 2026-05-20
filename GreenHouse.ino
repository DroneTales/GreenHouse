// LilyGO T-A7608E-H board
// =============================================================================
// Arduino IDE settings:
//   - Board: ESP32 WROVER Kit (all versions)
//   - CPU Frequency: 80MHz (WiFi/BT)
//   - Core Debug Level: None
//   - Erase All Flash Before Sketch Upload: Disabled
//   - Flash frequency: 80Mhz
//   - Flash Mode: QIO
//   - Flash Size: 4MB (32Mb)
//   - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
//   - PSRAM: Enabled
//   - Upload Speed: 921600
// =============================================================================

#include "Batt.h"
#include "Config.h"
#include "Modem.h"
#include "Mqtt.h"
#include "Sensors.h"

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_wifi.h>

#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>


/******************************************************************************/
/*                              Board  constants                              */

// The default sleep duration when configuration read failed. It allows user
// to insert SD card.
constexpr uint64_t DEFAULT_SLEEP_DURATION = (15 * 60 * 1000000);

/******************************************************************************/


/******************************************************************************/
/*                           Board helper functions                           */

// Disables Bluetooth and Wi-Fi units.
void DisableWirelessHardware()
{
    esp_bluedroid_disable();
    esp_bt_controller_disable();
    esp_wifi_stop();
}

// Hibernates the board.
void Hibernate(uint64_t SleepDuration)
{
    // Prepare board for going to deep sleep.
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);
    
    // Set the wakeup timer.
    esp_sleep_enable_timer_wakeup(SleepDuration);
    
    // Start deep sleep.
    esp_deep_sleep_start();
}

/******************************************************************************/


/******************************************************************************/
/*                           Data helper  functions                           */

// Collects data from battery and sensors.
void CollectData(SENSORS_CONFIG& SensorsConfig, BATTERY_INFO& BatteryInfo,
    SENSORS_DATA& SensorsData)
{
    // Get on-board battery data.
    BattGetInfo(BatteryInfo);
    // Then read temperature data.
    SensorsGetTemperature(SensorsConfig, SensorsData);
}

// Publishes data to the MQTT broker. Returns true if the data was published.
// Returns false otherwise.
bool PublishData(const CONFIG& Config, const BATTERY_INFO& BatteryInfo,
    const SENSORS_DATA& SensorsData)
{
    bool Result = false;

    // First, turn on the on-board modem unit.
    if (ModemTurnOn())
    {
        // Try to connect to a mobile network.
        if (ModemConnectToNetwork(Config.Gprs.Pin))
        {
            // Once network connect, try to connect to GPRS.
            if (ModemConnectToGprs(Config.Gprs))
            {
                // Ok, once we have internet connection we can try to
                // connect to MQTT broker.
                if (MqttConnect(Config.Mqtt))
                {
                    // Publish sensors data.
                    Result = MqttPublish(BatteryInfo, SensorsData);
                    
                    // Disconnect from MQTT broker.
                    MqttDisconnect();
                }
                // Disconnect from GPRS service.
                ModemDisconnectFromGprs();
            }
        }
        // Turns off the modem unit.
        ModemTurnOff();
    }
    
    return Result;
}

/******************************************************************************/


/******************************************************************************/
/*                             Arduino  functions                             */

void setup()
{
    // The very first thing we have to do is to disable brownout detection.
    uint32_t BownOutState = READ_PERI_REG(RTC_CNTL_BROWN_OUT_REG);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // Initialize the on-board battery.
    BattInit();
    // Initialize the on-board modem unit.
    ModemInit();
    // Initialize the temperature sensors hardware.
    SensorsInit();

    // Disable all wireless features because we do not use them.
    DisableWirelessHardware();
    
    // Power up the on-board hardware by enabling the on-board battery.
    BattEnable();
    
    // Set default sleep duration. We use it if the SD-card is not inserted
    // or if the configuration file was not found or is not valid.
    // This sleep duration allows a user to insert the SD-card.
    uint64_t SleepDuration = DEFAULT_SLEEP_DURATION;
    // Now read configuration.
    CONFIG Config;
    if (ConfigRead(Config))
    {
        // OK, configuration read with success. We can set sleep interval to
        // short one assuming data publishing failed.
        SleepDuration = Config.Board.ShortSleep;

        // Collect information from sensors.
        BATTERY_INFO BatteryInfo;
        SENSORS_DATA SensorsData;
        CollectData(Config.Sensors, BatteryInfo, SensorsData);

        // All data callected. We can try to send it to the MQTT broker.
        if (PublishData(Config, BatteryInfo, SensorsData))
            // If data was published then we can set the sleep duration
            // to a long period.
            SleepDuration = Config.Board.LongSleep;
    }

    // Turn off the on-board harware by disable the on-board battery
    // power source.
    BattDisable();
    
    // We must restore the brown-out detection state before goint
    // to sleep.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, BownOutState);
    
    // Hibernate the board for the specified period of time.
    Hibernate(SleepDuration);
}

void loop()
{
}

/******************************************************************************/
