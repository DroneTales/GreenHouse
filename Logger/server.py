import const
import db
import sensors

import sqlite3

from flask import Flask, render_template, jsonify, request
from waitress import serve


########################################################################
# Flask application setup.

# Create the Flask application instance.
app = Flask(__name__)
# Register the sensor management Blueprint
# (routes: /sensors, /api/sensors).
app.register_blueprint(sensors.sensor_bp)


def get_db():
    """
    Helper to obtain a new SQLite connection with rows returned as
    dictionary-like Row objects. This is used by the API endpoints below.
    """

    conn = sqlite3.connect(const.DATABASE_FILE_NAME)
    conn.row_factory = sqlite3.Row
    return conn

########################################################################


########################################################################
# Front‑end route.

@app.route('/')
def index():
    """
    Render the main dashboard page. This is the entry point of the web
    application – it loads index.html which likely includes JavaScript
    charts and controls for viewing sensor data.
    """

    return render_template('index.html')

########################################################################


########################################################################
# API endpoints.

# List all sensors.
@app.route('/api/sensors')
def api_sensors():
    """
    Return a JSON array of all sensor definitions, ordered by ID.
    Each object contains the sensor's "id" and "name".
    This data is used by the front‑end to populate dropdowns or labels.
    """

    conn = get_db()
    try:
        # Fetch all rows from the SENSORS table, sorted by ID.
        rows = conn.execute("select ID, NAME from SENSORS order by ID").fetchall()
        # Convert each sqlite3.Row to a plain dict for JSON serialisation.
        sensors = [{"id": r["ID"], "name": r["NAME"]} for r in rows]
        return jsonify(sensors)
    
    finally:
        # Ensure the connection is closed even if an exception occurs.
        conn.close()

# Fetch historical measurement data.
@app.route('/api/data')
def api_data():
    """
    Return a JSON array of measurement records within a given time interval.

    Query parameters:
        start – Unix timestamp (integer) beginning of the interval (inclusive)
        end   – Unix timestamp (integer) end of the interval (inclusive)

    Returns:
        200 – array of objects with keys: sensor_id, date_time, value
        400 – if "start" or "end" is missing
    """
    
    # Extract query parameters, cast to int.
    start = request.args.get("start", type = int)
    end = request.args.get("end", type = int)
    
    # Validate that both parameters are present.
    if start is None or end is None:
        return jsonify({"error": "start and end query parameters required"}), 400
    
    conn = get_db()
    try:
        # Fetch data points whose DATE_TIME falls within the requested range.
        rows = conn.execute("""
            select SENSOR_ID, DATE_TIME, VALUE
            from MQTT_DATA
            where DATE_TIME >= ? and DATE_TIME <= ?
            order by DATE_TIME""",
            (start, end)
        ).fetchall()

        # Build a list of dictionaries for the JSON response.
        data = []
        for r in rows:
            data.append({
                "sensor_id": r["SENSOR_ID"],
                "date_time": r["DATE_TIME"],
                "value": r["VALUE"]
            })
        return jsonify(data)
    
    finally:
        conn.close()

########################################################################


########################################################################
# Application entry point.

if __name__ == '__main__':
    # Make sure the database and tables exist before starting
    # the server.
    db.create_database()
    # Run the app using Waitress (production WSGI server)
    # on localhost:5000.
    serve(app, host = '127.0.0.1', port = 5000)

########################################################################
