import const
import settings

import paho.mqtt.client as mqtt
import sqlite3

from datetime import datetime


########################################################################################
# MQTT topics

MQTT_TOPIC_BATTERY_CAPACITY     = "greenhouse/battery"
MQTT_TOPIC_BATTERY_VOLTAGE      = "greenhouse/battery/voltage"

MQTT_TOPIC_TEMPERATURE_SENSOR   = "greenhouse/sensors/%u"

########################################################################################


########################################################################################
# Connects (or creates if not exists) to the database.

def create_database():
    connection = sqlite3.connect(const.DATABASE_FILE_NAME)
    cursor = connection.cursor()
    cursor.execute("""
        create table if not exists MQTT_DATA (
            DATE_TIME   integer not null default current_timestamp,
            DATA_TYPE   integer not null,
            VALUE       real
        )""")
    connection.commit()
    connection.close()

########################################################################################


########################################################################################
# Called when MQTT client connected to the broker.

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        # Subscribe to the topics
        for i in range(const.SENSORS_MAX_NUMBER):
            client.subscribe(MQTT_TOPIC_TEMPERATURE_SENSOR % i, settings.MQTT_QOS)

        client.subscribe(MQTT_TOPIC_BATTERY_CAPACITY, settings.MQTT_QOS)
        client.subscribe(MQTT_TOPIC_BATTERY_VOLTAGE, settings.MQTT_QOS)

########################################################################################


########################################################################################
# Called when a message is received

def on_message(client, userdata, msg):
    # Trace data.
    dateTime = datetime.now()
    date = dateTime.date()
    time = dateTime.time()

    # Convert topic name to the data type id.
    dataType = const.DATA_TYPE_UNDEFINED
    if (msg.topic == MQTT_TOPIC_BATTERY_CAPACITY) :
        dataType = const.DATA_TYPE_BATTERY_CAPACITY
    elif (msg.topic == MQTT_TOPIC_BATTERY_VOLTAGE) :
        dataType = const.DATA_TYPE_BATTERY_VOLTAGE
    else :
        for i in range(const.SENSORS_MAX_NUMBER) :
            topicName = MQTT_TOPIC_TEMPERATURE_SENSOR % i
            if (msg.topic == topicName) :
                dataType = const.DATA_TYPE_TEMPERATURE_SENSOR + i
                break
    
    # Add new data to the database.
    if (dataType != const.DATA_TYPE_UNDEFINED) :
        connection = sqlite3.connect("./greenhouse.db")
        cursor = connection.cursor()
        sql = f"insert into MQTT_DATA(DATA_TYPE, VALUE) values({dataType}, {msg.payload.decode()})"
        cursor.execute(sql)
        connection.commit()
        connection.close()

########################################################################################


########################################################################################
# MQTT logger.

def run_mqtt_client():
    # Create MQTT client instance.
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, settings.MQTT_CLIENT_ID, True)
    client.username_pw_set(settings.MQTT_USER_NAME, settings.MQTT_PASSWORD)

    # Assign callback functions.
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        # Connect to the broker.
        client.connect(settings.MQTT_SERVER, settings.MQTT_PORT, 60)
        # Start the network loop.
        client.loop_forever()
    
    except KeyboardInterrupt:
        client.disconnect()

    except Exception as e:
        client.disconnect()

########################################################################################


if __name__ == "__main__":
    create_database()
    run_mqtt_client()
