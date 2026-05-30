import const

import sqlite3


def create_database():
    """
    Initialise the SQLite database and create the required tables if they
    do not already exist. Also populates the SENSORS table with predefined
    sensor types (using INSERT OR IGNORE so that repeated calls are safe).
    """

    # Open a connection to the database file defined in const.py.
    connection = sqlite3.connect(const.DATABASE_FILE_NAME)
    # Enable Write-Ahead Logging for better concurrency and performance.
    connection.execute("PRAGMA journal_mode=WAL;")
    
    cursor = connection.cursor()

    # SENSORS table.
    # Stores the available sensor types together with a human‑readable name.
    # ID values are the same constants defined in const.py (DATA_TYPE_*).
    cursor.execute("""
        create table if not exists SENSORS (
            ID   integer not null primary key,
            NAME text    not null
        )""")
    # Insert the known sensor types. "insert or ignore" avoids errors if
    # the row already exists (e.g., on repeated initialisation).
    cursor.execute("insert or ignore into SENSORS(ID, NAME) values(1, 'Capacity')")
    cursor.execute("insert or ignore into SENSORS(ID, NAME) values(2, 'Voltage')")
    cursor.execute("insert or ignore into SENSORS(ID, NAME) values(100, 'Street')")
    cursor.execute("insert or ignore into SENSORS(ID, NAME) values(101, 'Left')")
    cursor.execute("insert or ignore into SENSORS(ID, NAME) values(102, 'Middle')")
    cursor.execute("insert or ignore into SENSORS(ID, NAME) values(103, 'Right')")
    
    # MQTT_DATA table.
    # Records every measurement received via MQTT.
    # DATE_TIME is stored as a Unix timestamp (UTC), automatically set to
    # the current time using SQLite's strftime('%s','now').
    # SENSOR_ID references SENSORS(ID) to enforce integrity.
    # VALUE is a real number holding the measurement.
    cursor.execute("""
        create table if not exists MQTT_DATA (
            DATE_TIME   integer not null default (strftime('%s', 'now')),
            SENSOR_ID   integer not null references SENSORS(ID),
            VALUE       real
        )""")
    
    # Save changes and close the database connection
    connection.commit()
    connection.close()
