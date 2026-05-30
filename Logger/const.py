########################################################################
# Database configuration.

# Path to the SQLite database file used for storing sensor data.
DATABASE_FILE_NAME              = "./greenhouse.db"

########################################################################


########################################################################
# Data type identifiers.
# These constants are used as "sensor IDs" in the MQTT_DATA table to
# distinguish what kind of measurement a record represents.

# Placeholder for unrecognized topics – no data will be stored
DATA_TYPE_UNDEFINED             = 0
# Battery capacity data received on the dedicated MQTT topic.
DATA_TYPE_BATTERY_CAPACITY      = 1
# Battery voltage data received on the dedicated MQTT topic.
DATA_TYPE_BATTERY_VOLTAGE       = 2
# Base identifier for temperature sensors. An index (0, 1, 2, ...) is
# added to this constant to derive the ID for each individual sensor.
# For example, sensor 0 → 100, sensor 1 → 101, and so on.
DATA_TYPE_TEMPERATURE_SENSOR    = 100

# Maximum number of temperature sensors that the system can handle.
# This determines how many MQTT topics will be subscribed to
# (greenhouse/sensors/0 through greenhouse/sensors/9).
SENSORS_MAX_NUMBER              = 10

########################################################################
