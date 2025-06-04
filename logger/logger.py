import paho.mqtt.client as mqtt
import sqlite3
import const

from datetime import datetime


########################################################################################
# MQTT Broker Settings
MQTT_SERVER     = "localhost"
MQTT_PORT       = 1883
MQTT_USER_NAME  = "" # Provide your MQTT server user name here.
MQTT_PASSWORD   = "" # Provide your MQTT server password here.
MQTT_CLIENT_ID  = "greenhouse-web"
MQTT_QOS        = 1

# MQTT topics
MQTT_TOPIC_AVG_TEMPERATURE          = "greenhouse/temperature"
MQTT_TOPIC_TEMPERATURE_SENSORS      = "greenhouse/sensors/%u"

MQTT_TOPIC_BATTERY_CAPACITY         = "greenhouse/battery"
MQTT_TOPIC_BATTERY_VOLTAGE          = "greenhouse/voltage/voltage"
MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE  = "greenhouse/voltage/adjusted"
########################################################################################


########################################################################################
# Connects (or creates if not exists) to the database.
def create_database():
    connection = sqlite3.connect(const.DATABASE_FILE_NAME)
    cursor = connection.cursor()
    cursor.execute("""
        create table if not exists SENSORS (
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
        print("Connected successfully to MQTT broker")

        # Subscribe to the topics
        client.subscribe(MQTT_TOPIC_AVG_TEMPERATURE, MQTT_QOS)
        for i in range(const.SENSOR_ZONES_COUNT):
            client.subscribe(MQTT_TOPIC_TEMPERATURE_SENSORS % i, MQTT_QOS)

        client.subscribe(MQTT_TOPIC_BATTERY_CAPACITY, MQTT_QOS)
        client.subscribe(MQTT_TOPIC_BATTERY_VOLTAGE, MQTT_QOS)
        client.subscribe(MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE, MQTT_QOS)
    else:
        print(f"Connection failed with code {reason_code}")
########################################################################################


########################################################################################
# Callback when a message is received
def on_message(client, userdata, msg):
    # Trace data.
    dateTime = datetime.now()
    date = dateTime.date()
    time = dateTime.time()
    print(f"[{date}  {time}]: Received message on topic {msg.topic}: {msg.payload.decode()}")

    # Convert topic name to the data type id.
    dataType = const.DATA_TYPE_UNDEFINED
    if (msg.topic == MQTT_TOPIC_AVG_TEMPERATURE) :
        dataType = const.DATA_TYPE_AVG_TEMPERATURE
    elif (msg.topic == MQTT_TOPIC_BATTERY_CAPACITY) :
        dataType = const.DATA_TYPE_BATTERY_CAPACITY
    elif (msg.topic == MQTT_TOPIC_BATTERY_VOLTAGE) :
        dataType = const.DATA_TYPE_BATTERY_VOLTAGE
    elif (msg.topic == MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE) :
        dataType = const.DATA_TYPE_ADJUSTED_VOLATGE
    else :
        for i in range(const.SENSOR_ZONES_COUNT) :
            topicName = MQTT_TOPIC_TEMPERATURE_SENSORS % i
            if (msg.topic == topicName) :
                dataType = const.DATA_TYPE_TEMPERATURE_SENSOR + i
                break
    
    # Add new data to the database.
    if (dataType != const.DATA_TYPE_UNDEFINED) :
        connection = sqlite3.connect("./greenhouse.db")
        cursor = connection.cursor()
        sql = f"insert into SENSORS(DATA_TYPE, VALUE) values({dataType}, {msg.payload.decode()})"
        cursor.execute(sql)
        connection.commit()
        connection.close()
########################################################################################


########################################################################################
# MQTT logger.
def run_mqtt_client():
    # Create MQTT client instance.
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, MQTT_CLIENT_ID, True)
    client.username_pw_set(MQTT_USER_NAME, MQTT_PASSWORD)

    # Assign callback functions.
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        # Connect to the broker.
        client.connect(MQTT_SERVER, MQTT_PORT, 60)
        print(f"Connecting to MQTT broker at {MQTT_SERVER}...")
    
        # Start the network loop.
        client.loop_forever()
    
    except KeyboardInterrupt:
        print("Disconnecting from MQTT broker...")
        client.disconnect()

    except Exception as e:
        print(f"An error occurred: {e}")
        client.disconnect()
########################################################################################


if __name__ == "__main__":
    # Create database.
    create_database()
    # Start MQTT client.
    run_mqtt_client()
