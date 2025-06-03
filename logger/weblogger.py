import paho.mqtt.client as mqtt
import sqlite3
import threading
import dash
import plotly.express as px
import pandas as pd
import sqlite3
import threading

from dash import dcc, html
from dash.dependencies import Input, Output
from datetime import datetime
from datetime import datetime, timedelta
from collections import OrderedDict


########################################################################################
# MQTT Broker Settings
MQTT_SERVER     = "localhost"
MQTT_PORT       = 1883
MQTT_USER_NAME  = "" # Provide your MQTT server user name here.
MQTT_PASSWORD   = "" # Provide your MQTT server password here.
MQTT_CLIENT_ID  = "greenhouse-web"
MQTT_QOS        = 1

# MQTT topics
SENSOR_ZONES_COUNT                  = 4
MQTT_TOPIC_AVG_TEMPERATURE          = "greenhouse/temperature"
MQTT_TOPIC_TEMPERATURE_SENSORS      = "greenhouse/sensors/%u"

MQTT_TOPIC_BATTERY_CAPACITY         = "greenhouse/battery"
MQTT_TOPIC_BATTERY_VOLTAGE          = "greenhouse/voltage/voltage"
MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE  = "greenhouse/voltage/adjusted"


# Data types stored in database.
DATA_TYPE_UNDEFINED             = 0
DATA_TYPE_BATTERY_CAPACITY      = 1
DATA_TYPE_BATTERY_VOLTAGE       = 2
DATA_TYPE_ADJUSTED_VOLATGE      = 3
DATA_TYPE_AVG_TEMPERATURE       = 4
DATA_TYPE_TEMPERATURE_SENSOR    = 5
########################################################################################


########################################################################################
# Connects (or creates if not exists) to the database.
def create_database():
    connection = sqlite3.connect("./greenhouse.db")
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
        for i in range(SENSOR_ZONES_COUNT):
            client.subscribe(MQTT_TOPIC_TEMPERATURE_SENSORS % i, MQTT_QOS)

        client.subscribe(MQTT_TOPIC_BATTERY_CAPACITY, MQTT_QOS)
        client.subscribe(MQTT_TOPIC_BATTERY_VOLTAGE, MQTT_QOS)
        client.subscribe(MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE, MQTT_QOS)
    else:
        print(f"Connection failed with code {reason_code}")


########################################################################################
# Callback when a message is received
def on_message(client, userdata, msg):
    # Trace data.
    dateTime = datetime.now()
    date = dateTime.date()
    time = dateTime.time()
    print(f"[{date}  {time}]: Received message on topic {msg.topic}: {msg.payload.decode()}")

    # Convert topic name to the data type id.
    dataType = DATA_TYPE_UNDEFINED
    if (msg.topic == MQTT_TOPIC_AVG_TEMPERATURE) :
        dataType = DATA_TYPE_AVG_TEMPERATURE
    elif (msg.topic == MQTT_TOPIC_BATTERY_CAPACITY) :
        dataType = DATA_TYPE_BATTERY_CAPACITY
    elif (msg.topic == MQTT_TOPIC_BATTERY_VOLTAGE) :
        dataType = DATA_TYPE_BATTERY_VOLTAGE
    elif (msg.topic == MQTT_TOPIC_BATTERY_AJUSTED_VOLTAGE) :
        dataType = DATA_TYPE_ADJUSTED_VOLATGE
    else :
        for i in range(SENSOR_ZONES_COUNT) :
            topicName = MQTT_TOPIC_TEMPERATURE_SENSORS % i
            if (msg.topic == topicName) :
                dataType = DATA_TYPE_TEMPERATURE_SENSOR + i
                break
    
    # Add new data to the database.
    if (dataType != DATA_TYPE_UNDEFINED) :
        connection = sqlite3.connect("./greenhouse.db")
        cursor = connection.cursor()
        sql = f"insert into SENSORS(DATA_TYPE, VALUE) values({dataType}, {msg.payload.decode()})"
        cursor.execute(sql)
        connection.commit()
        connection.close()
########################################################################################


########################################################################################
# The MQTT client thread
def run_mqtt_client():
    # Create MQTT client instance
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, MQTT_CLIENT_ID, True)
    client.username_pw_set(MQTT_USER_NAME, MQTT_PASSWORD)

    # Assign callback functions
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        # Connect to the broker
        client.connect(MQTT_SERVER, MQTT_PORT, 60)
        print(f"Connecting to MQTT broker at {MQTT_SERVER}...")
    
        # Start the network loop
        client.loop_forever()
    
    except KeyboardInterrupt:
        print("Disconnecting from MQTT broker...")
        client.disconnect()

    except Exception as e:
        print(f"An error occurred: {e}")
        client.disconnect()
########################################################################################


########################################################################################
# Web Dashboard Setup
def create_dashboard_app():
    app = dash.Dash(__name__)
    
    # Define the layout with buttons
    app.layout = html.Div([
        html.H1("Greenhouse Sensor Dashboard"),
        
        # Button row for time selection
        html.Div([
            html.Button("24 Hours", id = "btn-1d",  n_clicks = 0, className = "time-btn"),
            html.Button("7 Days",   id = "btn-7d",  n_clicks = 0, className = "time-btn"),
            html.Button("30 Days",  id = "btn-30d", n_clicks = 0, className = "time-btn"),
            html.Button("1 Year",   id = "btn-1y",  n_clicks = 0, className = "time-btn"),
        ], className="button-row"),
        
        # Graphs
        dcc.Graph(id = "temperature-plot"),
        dcc.Graph(id = "battery-plot"),
        
        # Auto-update component
        dcc.Interval(
            id          = "interval-component",
            interval    = 60 * 1000,  # Update every minute
            n_intervals = 0
        ),
        
        # Hidden storage for current time range
        html.Div(id = "time-range-store", style = {"display": "none"}, children = "24h"),
    ])
    
    # Add CSS for styling
    app.css.append_css({
        "external_url": "https://codepen.io/chriddyp/pen/bWLwgP.css"
    })
    
    # Helper function to get data from database
    def get_sensor_data(time_range = "24h", start_date = None, end_date = None):
        conn = sqlite3.connect("./greenhouse.db")
        
        # Calculate time range
        if time_range == "24h":
            cutoff = datetime.now() - timedelta(hours = 24)
        elif time_range == "7d":
            cutoff = datetime.now() - timedelta(days = 7)
        elif time_range == "30d":
            cutoff = datetime.now() - timedelta(days = 30)
        elif time_range == "1y":
            cutoff = datetime.now() - timedelta(days = 365)
        else:
            cutoff = datetime.now() - timedelta(hours = 24)
        
        query = """
            SELECT DATE_TIME, DATA_TYPE, VALUE 
            FROM SENSORS 
            WHERE DATE_TIME >= ?
            ORDER BY DATE_TIME
        """
        params = (cutoff.timestamp(),)
        
        df = pd.read_sql(query, conn, params = params)
        conn.close()
        
        # Map data types to human-readable names
        type_mapping = {
            DATA_TYPE_AVG_TEMPERATURE:  "Average Temperature",
            DATA_TYPE_BATTERY_CAPACITY: "Battery Capacity",
            DATA_TYPE_BATTERY_VOLTAGE:  "Battery Voltage",
            DATA_TYPE_ADJUSTED_VOLATGE: "Adjusted Voltage",
        }
        
        # Add temperature sensors
        for i in range(SENSOR_ZONES_COUNT):
            type_mapping[DATA_TYPE_TEMPERATURE_SENSOR + i] = f"Zone {i} Temperature"
        
        df["Sensor"] = df["DATA_TYPE"].map(type_mapping)
        
        # Convert timestamp
        try:
            df["DATE_TIME"] = pd.to_datetime(df["DATE_TIME"], unit = "s")
        except ValueError:
            df["DATE_TIME"] = pd.to_datetime(df["DATE_TIME"])
        
        return df
    
    # Callback to update time range
    @app.callback(
        Output("time-range-store",  "children"),
        [Input("btn-1d",            "n_clicks"),
         Input("btn-7d",            "n_clicks"),
         Input("btn-30d",           "n_clicks"),
         Input("btn-1y",            "n_clicks")]
    )
    def update_time_range(btn1, btn7, btn30, btn1y):
        ctx = dash.callback_context
        
        if not ctx.triggered:
            return "24h"
        
        button_id = ctx.triggered[0]["prop_id"].split(".")[0]
        
        if button_id == "btn-1d":
            return "24h"
        elif button_id == "btn-7d":
            return "7d"
        elif button_id == "btn-30d":
            return "30d"
        elif button_id == "btn-1y":
            return "1y"
        
        return "24h"
    
    # Callback for updating plots
    @app.callback(
        [Output("temperature-plot",     "figure"),
         Output("battery-plot",         "figure")],
        [Input("time-range-store",      "children"),
         Input("interval-component",    "n_intervals")]
    )
    def update_plots(time_range, _):
        df = get_sensor_data(time_range)
        time_label = {
            "24h":  "24 Hours",
            "7d":   "7 Days",
            "30d":  "30 Days",
            "1y":   "1 Year"
        }.get(time_range, "24 Hours")
        
        # Create temperature plot
        temp_df = df[df["Sensor"].str.contains("Temperature")]
        temp_fig = px.line(
            temp_df, 
            x       = "DATE_TIME", 
            y       = "VALUE", 
            color   = "Sensor",
            title   = f"Temperature Sensors - Last {time_label}",
            labels  = {"VALUE": "Temperature (°C)", "DATE_TIME": "Time"}
        )
        
        # Create battery plot
        battery_df = df[df["Sensor"].str.contains("Battery|Voltage")]
        battery_fig = px.line(
            battery_df, 
            x       = "DATE_TIME", 
            y       = "VALUE", 
            color   = "Sensor",
            title   = f"Battery Status - Last {time_label}",
            labels  = {"VALUE": "Value", "DATE_TIME": "Time"}
        )
        
        return temp_fig, battery_fig
    
    return app


########################################################################################
# The dashboard server thread
def run_dashboard():
    app = create_dashboard_app()
    app.run(host = "0.0.0.0", port = 8228, debug = False)
########################################################################################


if __name__ == "__main__":
    create_database()

    # Start MQTT client in a thread
    mqttThread = threading.Thread(target = run_mqtt_client, daemon = True)
    mqttThread.start()

    # Start dashboard in a thread
    dashboardThread = threading.Thread(target = run_dashboard, daemon = True)
    dashboardThread.start()

    print("Both MQTT client and dashboard are running in background threads.")
    print(f"Dashboard available at: http://localhost:8050")

    # Keep main thread alive
    try:
        while True:
            pass
    except KeyboardInterrupt:
        print("\nShutting down...")