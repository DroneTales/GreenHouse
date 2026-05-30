import const
import db
import settings

import paho.mqtt.client as mqtt
import sqlite3

from datetime import datetime

########################################################################
# MQTT topic definitions.

# Topic for receiving battery capacity data.
MQTT_TOPIC_BATTERY_CAPACITY     = "greenhouse/battery"
# Topic for receiving battery voltage data.
MQTT_TOPIC_BATTERY_VOLTAGE      = "greenhouse/battery/voltage"

# Template topic for temperature sensors; %u will be replaced by
# sensor index.
MQTT_TOPIC_TEMPERATURE_SENSOR   = "greenhouse/sensors/%u"

########################################################################


def on_connect(client, userdata, flags, reason_code, properties):
    """
    Callback executed when the MQTT client connects (or fails to
    connect) to the broker.

    Args:
        client:     MQTT client instance
        userdata:   user-defined data passed to the client
        flags:      connection flags
        reason_code: integer result code (0 = success)
        properties: MQTT v5 properties (may be None)
    """

    # If connection was successful, subscribe to all needed topics.
    if reason_code == 0:
        # Subscribe to each possible temperature sensor topic.
        for i in range(const.SENSORS_MAX_NUMBER):
            client.subscribe(MQTT_TOPIC_TEMPERATURE_SENSOR % i, settings.MQTT_QOS)
        
        # Subscribe to battery capacity topic.
        client.subscribe(MQTT_TOPIC_BATTERY_CAPACITY, settings.MQTT_QOS)
        # Subscribe to battery voltage topic.
        client.subscribe(MQTT_TOPIC_BATTERY_VOLTAGE, settings.MQTT_QOS)


def on_message(client, userdata, msg):
    """
    Callback executed when a message is received on a subscribed topic.

    Args:
        client:   MQTT client instance
        userdata: user-defined data
        msg:      MQTTMessage object with topic, payload, qos, retain, etc.
    """

    # Default data type indicates "undefined" if topic not recognised.
    dataType = const.DATA_TYPE_UNDEFINED

    # Determine the sensor/data type based on the received topic.
    if (msg.topic == MQTT_TOPIC_BATTERY_CAPACITY):
        dataType = const.DATA_TYPE_BATTERY_CAPACITY
    elif (msg.topic == MQTT_TOPIC_BATTERY_VOLTAGE):
        dataType = const.DATA_TYPE_BATTERY_VOLTAGE
    else:
        # Check if message belongs to a temperature sensor.
        for i in range(const.SENSORS_MAX_NUMBER):
            topicName = MQTT_TOPIC_TEMPERATURE_SENSOR % i
            if (msg.topic == topicName):
                # Sensor ID = base temperature constant + sensor index.
                dataType = const.DATA_TYPE_TEMPERATURE_SENSOR + i
                break
    
    # Only store data if we have a valid data type.
    if (dataType != const.DATA_TYPE_UNDEFINED):
        # Connect to the SQLite database.
        connection = sqlite3.connect("./greenhouse.db")
        cursor = connection.cursor()

        # Build and execute INSERT statement using the decoded payload.
        sql = f"insert into MQTT_DATA(SENSOR_ID, VALUE) values({dataType}, {msg.payload.decode()})"
        cursor.execute(sql)

        # Commit changes and close the connection.
        connection.commit()
        connection.close()


def run_mqtt_client():
    """
    Initialise the MQTT client, register callbacks, connect to the broker,
    and start a blocking network loop to receive messages indefinitely.
    """

    # Create client instance using callback API version 2.
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, settings.MQTT_CLIENT_ID, True)
    # Set username and password for broker authentication.
    client.username_pw_set(settings.MQTT_USER_NAME, settings.MQTT_PASSWORD)
    
    # Assign the callback functions.
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        # Connect to the MQTT broker.
        client.connect(settings.MQTT_SERVER, settings.MQTT_PORT, 60)
        # Start the network loop; this call blocks and processes messages forever.
        client.loop_forever()
    
    except KeyboardInterrupt:
        # Gracefully disconnect on Ctrl+C.
        client.disconnect()
    
    except Exception as e:
        # Handle any other unexpected errors and disconnect.
        client.disconnect()


if __name__ == "__main__":
    # Ensure the database and required tables exist before starting the client.
    db.create_database()
    # Launch the MQTT client (will run until interrupted).
    run_mqtt_client()
