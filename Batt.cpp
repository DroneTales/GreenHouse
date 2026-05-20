#include "Batt.h"
#include "Pins.h"


/******************************************************************************/
/*                         On-board battery constants                         */

// The voltage divider. By schematic it is 1:2
// but actual value may vary. In my case it is 1:1.95
constexpr float BATTERY_VOLTAGE_DIVIDER = 1.95;
// The empty battery voltage.
constexpr float BATTERY_VOLTS_EMPTY = 3.4;
// The full battery voltage.
constexpr float BATTERY_VOLTS_FULL = 4.19;
// Low battery capacity.
constexpr uint8_t BATTERY_LOW_CAPACITY = 10;

/******************************************************************************/


/******************************************************************************/
/*                         On-board battery functions                         */

// Initializes the on-board battery pins.
void BattInit()
{
    // Initialize the on-board battery and switch it off so all on-board
    // hardware (SD-card reader and modem) will be turned off (in case if
    // the board is powered from the on-board battery).
    pinMode(BATT_18650_EN_PIN, OUTPUT);
    pinMode(BATT_18650_ADC_PIN, INPUT);
    // Turn off the on-board power supply.
    digitalWrite(BATT_18650_EN_PIN, LOW);
}

// Enables the on-board battery power source.
void BattEnable()
{
    digitalWrite(BATT_18650_EN_PIN, HIGH);
}

// Reads the on-board battery information.
void BattGetInfo(BATTERY_INFO& Info)
{
    // Read battery voltage. Following by the board schematics there is 1:2
    // voltage devider on the battery ADC pin.
    Info.Voltage = (float)analogReadMilliVolts(BATT_18650_ADC_PIN) /
        1000.0 * BATTERY_VOLTAGE_DIVIDER;
    
    // Calculate battery capacity
    float AdjustedVoltage = Info.Voltage;
    if (AdjustedVoltage < BATTERY_VOLTS_EMPTY)
        AdjustedVoltage = BATTERY_VOLTS_EMPTY;
    if (AdjustedVoltage > BATTERY_VOLTS_FULL)
        AdjustedVoltage = BATTERY_VOLTS_FULL;

    // Calculate capacity.
    Info.Capacity = (uint8_t)(100.0 * ((AdjustedVoltage - BATTERY_VOLTS_EMPTY) /
        (BATTERY_VOLTS_FULL - BATTERY_VOLTS_EMPTY)));
}

// Disables the on-board battery power source.
void BattDisable()
{
    digitalWrite(BATT_18650_EN_PIN, LOW);
}

/******************************************************************************/
