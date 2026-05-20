#include "Config.h"
#include "Pins.h"

#include <FS.h>
#include <SD.h>
#include <SPI.h>


/******************************************************************************/
/*                          Configuration  constants                          */

// The configuration INI file name.
const char* const INI_FILE_NAME = "/config.txt";

/*                          INI file section  names                          */

// The board configuration section name.
const char* const SECTION_BOARD = "Board";
// GPRS configuration section name.
const char* const SECTION_GPRS = "GPRS";
// MQTT section name.
const char* const SECTION_MQTT = "MQTT";
// Temperature sensors section name.
const char* const SECTION_SENSORS = "Sensors";

/*                      INI file board section key names                      */

// The long sleep (success or no config file) interval key name.
const char* const KEY_BOARD_LONG_SLEEP = "LongSleep";
// The short sleep (failed) interval key name.
const char* const KEY_BOARD_SHORT_SLEEP = "ShortSleep";

/*                      INI file GPRS section  key names                      */

// The APN key name.
const char* const KEY_GPRS_APN = "APN";
// The password key name.
const char* const KEY_GPRS_PASSWORD = "Password";
// The user name key name.
const char* const KEY_GPRS_USER_NAME = "UserName";
// The SIM PIN key name.
const char* const KEY_GPRS_SIM_PIN = "Pin";

/*                      INI file MQTT section  key names                      */

// MQTT client ID key name.
const char* const KEY_MQTT_CLIENT_ID = "ClientID";
// The password key name.
const char* const KEY_MQTT_PASSWORD = "Password";
// MQTT broker key name.
const char* const KEY_MQTT_SERVER_NAME = "Server";
// MQTT broker port key name,
const char* const KEY_MQTT_SERVER_PORT = "Port";
// The user name key name.
const char* const KEY_MQTT_USER_NAME = "UserName";

/*                     INI file sensors section key names                     */

// Number of sensors key name.
const char* const KEY_SENSORS_COUNT = "Count";
// Sensor address key name.
const char* const KEY_SENSORS_ADDRESS = "Address_%u";

/******************************************************************************/


/******************************************************************************/
/*                   Configuration SD-card helper functions                   */

// Mounts the SD-card. Returns true if the SD-card found. Returns
// false otherwise.
bool Mount()
{
	// Set SD card reader pins.
    pinMode(TF_CARD_CS_PIN, OUTPUT);
    digitalWrite(TF_CARD_CS_PIN, LOW);

    // Initialize SPI bus.
    if (!SPI.begin(TF_CARD_SCLK_PIN, TF_CARD_MISO_PIN, TF_CARD_MOSI_PIN, TF_CARD_CS_PIN))
        return false;

	// Mount SD card.
    if (!SD.begin(TF_CARD_CS_PIN, SPI))
    {
		// If card is not inserted then release the SPI bus.
		SPI.end();
		// And return error.
		return false;
	}
	
	// It looks like card is inserted.
	return true;
}

// Unmount the SD-card.
void Unmount()
{
	// Release the card reader.
	SD.end();
	// Release the SPI bus.
	SPI.end();
}

/******************************************************************************/


/******************************************************************************/
/*                  Configuration INI file helper  functions                  */

// Returns true if the specified buffer is empty string.
bool IsEmpty(const char* const Buffer)
{
    return (Buffer[0] == '\0');
}

// Returns true if the string in the specified buffer is a comment.
bool IsComment(const char* const Buffer)
{
    return (Buffer[0] == '#' || Buffer[0] == ';');
}

// Returns true if the string is a section name.
bool IsSection(const char* const Buffer)
{
    return (Buffer[0] == '[' || Buffer[0] == ']');
}

// Returns true if the string is a key-value pair.
bool IsKeyValuePair(const char* const Buffer)
{
    return (strlen(Buffer) > 1 && strchr(Buffer, '=') != NULL &&
            Buffer[0] != '=');
}

// Returns true if the string is a valid section name.
bool IsValidSection(const char* const Buffer)
{
    size_t Len = strlen(Buffer);
    return (Len > 2 && Buffer[0] == '[' && Buffer[Len - 1] == ']');
}

// Clears the string.
void Clear(char* const Buffer)
{
    Buffer[0] = '\0';
}

// Removes tralling and ending spaces from the specified string.
void Trim(char* const Buffer)
{
    if (IsEmpty(Buffer))
        return;
    
    size_t Pos = 0;
    size_t Len = strlen(Buffer);
    while (isspace(Buffer[Pos]) && Pos < Len)
        Pos++;
    if (Pos == Len)
    {
        Clear(Buffer);
        return;
    }
    strcpy(Buffer, &Buffer[Pos]);
    
    Pos = strlen(Buffer) - 1;
    while (isspace(Buffer[Pos]) && Pos > 0)
        Pos--;
    Buffer[Pos + 1] = '\0';
}

// Removes unnecessary spaces.
void NormalizeSection(char* const Buffer)
{
    size_t Len = strlen(Buffer);
    if (Buffer[0] == ']' || Len < 3)
    {
        Clear(Buffer);
        return;
    }
    
    size_t Pos = 1;
    while (Pos < Len && isspace(Buffer[Pos]))
        Pos++;
    if (Pos == Len)
    {
        Clear(Buffer);
        return;
    }
    
    if (Pos > 1)
    {
        strcpy(&Buffer[1], &Buffer[Pos]);
        if (strlen(Buffer) < 3)
        {
            Clear(Buffer);
            return;
        }
    }
    
    Len = strlen(Buffer);
    for (size_t i = 1; i < Len; i++)
    {
        if (Buffer[i] == '[')
        {
            Clear(Buffer);
            return;
        }
    }
    
    if (Buffer[Len - 1] != ']')
    {
        Clear(Buffer);
        return;
    }
    
    for (size_t i = Len - 2; i > 0; i--)
    {
        if (Buffer[i] == ']')
        {
            Clear(Buffer);
            return;
        }
    }
    
    Pos = Len - 2;
    while (Pos > 0 && isspace(Buffer[Pos]))
        Pos--;
    if (Pos == 0)
    {
        Clear(Buffer);
        return;
    }
    Buffer[Pos + 1] = ']';
    Buffer[Pos + 2] = '\0';
}

// Removes unnecessary spaces in key-value pair string.
void NormalizeKeyValuePair(char* const Buffer)
{
    size_t EqPos = 0;
    while (EqPos < strlen(Buffer) && Buffer[EqPos] != '=')
        EqPos++;
    if (isspace(Buffer[EqPos - 1]))
    {
        size_t KeyPos = EqPos - 1;
        while (KeyPos > 0 && isspace(Buffer[KeyPos]))
            KeyPos--;
        if (isspace(Buffer[KeyPos + 1]))
            strcpy(&Buffer[KeyPos + 1], &Buffer[EqPos]);
        EqPos = KeyPos + 1;
    }

    size_t Pos = EqPos + 1;
    while (Pos < strlen(Buffer) && isspace(Buffer[Pos]))
        Pos++;
    if (Pos > EqPos + 1)
        strcpy(&Buffer[EqPos + 1], &Buffer[Pos]);
    Pos = EqPos + 1;
    while (Pos < strlen(Buffer))
    {
        if (Buffer[Pos] == '=')
        {
            Clear(Buffer);
            break;
        }
        Pos++;
    }
}

// Normilizes the string.
void Normalize(char* const Buffer)
{
    if (IsComment(Buffer))
        return;
    if (IsSection(Buffer))
    {
        NormalizeSection(Buffer);
        return;
    }
    if (!IsKeyValuePair(Buffer))
    {
        Clear(Buffer);
        return;
    }
    NormalizeKeyValuePair(Buffer);
}

// Reads a single line from the specified file.
bool ReadLine(File& f, char* const Buffer)
{
    unsigned char Pos = 0;
    unsigned char ReadBytes = 0;
    while (f.available())
    {
        char c = f.read();
        ReadBytes++;
        if (c == '\r' || c == '\n')
        {
            char NextNewLine = (c == '\r' ? '\n' : '\r');
            if (f.available())
            {
                c = f.read();
                if (c != NextNewLine)
                    f.seek(f.position() - 1);
            }
            break;
        }
        Buffer[Pos] = c;
        Pos++;
    }
    Buffer[Pos] = '\0';
    if (ReadBytes > 0)
    {
        Trim(Buffer);
        Normalize(Buffer);
    }
    return (ReadBytes > 0);
}

// Extracts the section name (removes brakets).
void ExtractSectionName(char* const Buffer)
{
    strcpy(Buffer, &Buffer[1]);
    Buffer[strlen(Buffer) - 1] = '\0';
}

// Gets the key name from the key-value pair string.
void GetKey(const char* const Buffer, char* const Key)
{
    size_t Pos = 0;
    while (Pos < strlen(Buffer) && Buffer[Pos] != '=')
    {
        Key[Pos] = Buffer[Pos];
        Pos++;
    }
}

// Gets the value from the key-value string.
void GetValue(const char* const Buffer, char* const Value)
{
    size_t Pos = 0;
    size_t Len = strlen(Buffer);
    while (Pos < strlen(Buffer) && Buffer[Pos] != '=')
        Pos++;
    if (Pos + 1 == Len)
        return;
    strcpy(Value, &Buffer[Pos + 1]);
}

// Reads value for the specified key in the specified section.
bool GetValue(File& f, const char* const Section, const char* const Key,
              char* const Value)
{
    Clear(Value);
    
    f.seek(0);
    char Buffer[128] = { 0 };
    while (ReadLine(f, Buffer))
    {
        if (IsEmpty(Buffer))
            continue;
        if (!IsValidSection(Buffer))
            continue;
        
        ExtractSectionName(Buffer);
        if (strcasecmp(Section, Buffer) != 0)
            continue;
        
        while (ReadLine(f, Buffer))
        {
            if (IsEmpty(Buffer))
                continue;
            if (IsValidSection(Buffer))
                continue;
            
            char CurrentKey[128] = { 0 };
            GetKey(Buffer, CurrentKey);
            if (strcasecmp(Key, CurrentKey) != 0)
                continue;
            
            GetValue(Buffer, Value);
            return true;
        }
    }
    return false;
}

// Reads board configuration from the specified file. Returns true if
// configuration read. Returns false otherwise.
bool ReadBoardConfig(File& f, BOARD_CONFIG& Config)
{
    char Value[128] = { 0 };
    char* endptr;

    if (!GetValue(f, SECTION_BOARD, KEY_BOARD_SHORT_SLEEP, Value))
        return false;
    if (IsEmpty(Value))
        return false;
    endptr = NULL;
    errno = 0;
    uint64_t ShortSleep = strtoull(Value, &endptr, 10);
    if (ShortSleep == 0 || errno == ERANGE || endptr == Value)
        return false;

    if (!GetValue(f, SECTION_BOARD, KEY_BOARD_LONG_SLEEP, Value))
        return false;
    if (IsEmpty(Value))
        return false;
    endptr = NULL;
    errno = 0;
    uint64_t LongSleep = strtoull(Value, NULL, 10);
    if (LongSleep == 0 || errno == ERANGE || endptr == Value)
        return false;
    
    Config.ShortSleep = ShortSleep;
    Config.LongSleep = LongSleep;
    return true;
}

// Reads GPRS configuration from the specified file. Returns true if
// configuration read. Returns false otherwise.
bool ReadGprsConfig(File& f, GPRS_CONFIG& Config)
{
    char Value[128] = { 0 };

    if (!GetValue(f, SECTION_GPRS, KEY_GPRS_APN, Value))
        return false;
    if (strlen(Value) > GPRS_APN_LEN)
        return false;
    if (!IsEmpty(Value))
        strcpy(Config.Apn, Value);

    if (!GetValue(f, SECTION_GPRS, KEY_GPRS_USER_NAME, Value))
        return false;
    if (strlen(Value) > GPRS_USER_NAME_LEN)
        return false;
    if (!IsEmpty(Value))
        strcpy(Config.UserName, Value);

    if (!GetValue(f, SECTION_GPRS, KEY_GPRS_PASSWORD, Value))
        return false;
    if (strlen(Value) > GPRS_PASSWORD_LEN)
        return false;
    if (!IsEmpty(Value))
        strcpy(Config.Password, Value);

    if (!GetValue(f, SECTION_GPRS, KEY_GPRS_SIM_PIN, Value))
        return false;
    if (strlen(Value) > GPRS_SIM_PIN_LEN)
        return false;
    if (!IsEmpty(Value))
        strcpy(Config.Pin, Value);

    return true;
}

// Reads MQTT server (broker) configuration from the specified file.
// Returns true if configuration read. Returns false otherwise.
bool ReadMqttConfig(File& f, MQTT_CONFIG& Config)
{
    char Value[128] = { 0 };

    if (!GetValue(f, SECTION_MQTT, KEY_MQTT_SERVER_NAME, Value))
        return false;
    if (strlen(Value) > MQTT_SERVER_NAME_LEN)
        return false;
    if (IsEmpty(Value))
        return false;
    strcpy(Config.Server, Value);

    if (!GetValue(f, SECTION_MQTT, KEY_MQTT_SERVER_PORT, Value))
        return false;
    if (IsEmpty(Value))
        return false;
    char* endptr = NULL;
    errno = 0;
    unsigned long Port = strtoul(Value, &endptr, 10);
    if (Port == 0 || Port > 0xFFFF || errno == ERANGE || endptr == Value)
        return false;
    Config.Port = (uint16_t)Port;

    if (!GetValue(f, SECTION_MQTT, KEY_MQTT_USER_NAME, Value))
        return false;
    if (strlen(Value) > MQTT_USER_NAME_LEN)
        return false;
    if (!IsEmpty(Value))
        strcpy(Config.UserName, Value);

    if (!GetValue(f, SECTION_MQTT, KEY_MQTT_PASSWORD, Value))
        return false;
    if (strlen(Value) > MQTT_PASSWORD_LEN)
        return false;
    if (!IsEmpty(Value))
        strcpy(Config.Password, Value);

    if (!GetValue(f, SECTION_MQTT, KEY_MQTT_CLIENT_ID, Value))
        return false;
    if (strlen(Value) > MQTT_CLIENT_ID_LEN)
        return false;
    if (!IsEmpty(Value))
        strcpy(Config.ClientId, Value);

    return true;
}

bool ReadSensorsConfig(File& f, SENSORS_CONFIG& Config)
{
    delay(200);

    char Value[128] = { 0 };

    if (!GetValue(f, SECTION_SENSORS, KEY_SENSORS_COUNT, Value))
        return false;
    if (IsEmpty(Value))
        return false;
    char* endptr = NULL;
    errno = 0;
    unsigned long Count = strtoul(Value, &endptr, 10);
    if (Count == 0 || Count > SENSORS_MAX_NUMBER || errno == ERANGE || endptr == Value)
        return false;
    Config.Count = (uint8_t)Count;

    for (uint8_t i = 0; i < Count; i++)
    {
        char Key[128] = { 0 };
        sprintf(Key, KEY_SENSORS_ADDRESS, i);
        if (!GetValue(f, SECTION_SENSORS, Key, Value))
            return false;
        if (IsEmpty(Value))
            return false;
        endptr = NULL;
        errno = 0;
        Config.Address[i] = strtoull(Value, &endptr, 10);
        if (Config.Address[i] == 0 || errno == ERANGE || endptr == Value)
            return false;
    }

    return true;
}

bool ReadConfig(File& f, CONFIG& Config)
{
    // Read board configuration from the file.
    if (!ReadBoardConfig(f, Config.Board))
        return false;
    // Read GPRS configuration from the file.
    if (!ReadGprsConfig(f, Config.Gprs))
        return false;
    // Read MQTT configuration.
    if (!ReadMqttConfig(f, Config.Mqtt))
        return false;
    // Read sensor addresses.
    return ReadSensorsConfig(f, Config.Sensors);
}

/******************************************************************************/


/******************************************************************************/
/*                          Configuration  functions                          */

// Reads configuration from SD-card. Returns true if the configuration is
// valid. Returns false otherwise.
bool ConfigRead(CONFIG& Config)
{
    // Clear configuration.
    memset(&Config, 0, sizeof(CONFIG));
    
    // Try to mount the SD-card.
    if (!Mount())
        return false;
    
    // Make sure the configuration file exists.
    bool Result = false;
    if (SD.exists(INI_FILE_NAME))
    {
        // Try to open the configuration file.
        File f = SD.open(INI_FILE_NAME, FILE_READ);
        if (f != NULL)
        {
            // Read all configuration.
            Result = ReadConfig(f, Config);
            // Do not forget to close the file.
            f.close();
        }
	}
    // Do not forget to unmount the SD-card. We will not use it any more.
    Unmount();

    return Result;
}

/******************************************************************************/
