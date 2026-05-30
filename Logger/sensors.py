import sqlite3
import const

from flask import Blueprint, jsonify, request, render_template


########################################################################
# Blueprint setup.

# Create a Blueprint named 'sensor_bp' to group all sensor-related routes.
# This Blueprint will be registered later in the main Flask application.

sensor_bp = Blueprint('sensor_bp', __name__)

########################################################################


def get_db():
    """
    Helper function to open a new SQLite connection.
    The connection is configured to return rows as sqlite3.Row objects,
    allowing access to columns by name (e.g., row['NAME']).
    """

    conn = sqlite3.connect(const.DATABASE_FILE_NAME)
    conn.row_factory = sqlite3.Row
    return conn


########################################################################
# Front‑end page.

@sensor_bp.route('/sensors')
def sensors_page():
    """
    Render the sensors management HTML page.
    This page likely provides a UI for adding, updating, and deleting
    sensors.
    """

    return render_template('sensors.html')

########################################################################


########################################################################
# API endpoints.

@sensor_bp.route('/api/sensors', methods=['POST'])
def api_add_sensor():
    """
    Add a new sensor to the SENSORS table.

    Expected JSON body:
        {
            "id": <integer>,   # Must be a numeric sensor ID
            "name": <string>   # Non‑empty sensor name
        }

    Returns:
        201 – sensor successfully added
        400 – missing or invalid input
        409 – sensor with the given ID already exists
        500 – database error
    """

    data = request.get_json()
    # Validate presence of required fields.
    if not data or 'id' not in data or 'name' not in data:
        return jsonify({"error": "id and name are required"}), 400
    
    sensor_id = int(data['id'])
    name = data['name'].strip()
    if not name:
        return jsonify({"error": "Name cannot be empty"}), 400

    conn = get_db()
    try:
        # Check for duplicate ID.
        existing = conn.execute("SELECT 1 FROM SENSORS WHERE ID = ?", (sensor_id,)).fetchone()
        if existing:
            return jsonify({"error": f"Sensor with ID {sensor_id} already exists"}), 409
        
        # Insert the new sensor.
        conn.execute("INSERT INTO SENSORS (ID, NAME) VALUES (?, ?)", (sensor_id, name))
        conn.commit()
        return jsonify({"message": "Sensor added", "id": sensor_id, "name": name}), 201
    
    except sqlite3.Error as e:
        return jsonify({"error": str(e)}), 500
    
    finally:
        conn.close()


@sensor_bp.route('/api/sensors/<int:sensor_id>', methods=['PUT'])
def api_update_sensor(sensor_id):
    """
    Update the name of an existing sensor.

    URL parameter:
        sensor_id – integer ID of the sensor to update

    Expected JSON body:
        {
            "name": <string>   # New non‑empty name for the sensor
        }

    Returns:
        200 – sensor successfully updated
        400 – missing or empty name
        404 – sensor not found
        500 – database error
    """
        
    data = request.get_json()
    if not data or 'name' not in data:
        return jsonify({"error": "name is required"}), 400
    
    name = data['name'].strip()
    if not name:
        return jsonify({"error": "Name cannot be empty"}), 400

    conn = get_db()
    try:
        # Attempt to update the sensor row.
        cur = conn.execute("UPDATE SENSORS SET NAME = ? WHERE ID = ?", (name, sensor_id))
        if cur.rowcount == 0:
            return jsonify({"error": "Sensor not found"}), 404
        
        conn.commit()
        return jsonify({"message": "Sensor updated", "id": sensor_id, "name": name})
    
    except sqlite3.Error as e:
        return jsonify({"error": str(e)}), 500
    
    finally:
        conn.close()


@sensor_bp.route('/api/sensors/<int:sensor_id>', methods=['DELETE'])
def api_delete_sensor(sensor_id):
    """
    Delete a sensor from the SENSORS table.

    The operation is only allowed if there are no associated data points
    in the MQTT_DATA table, to prevent orphaned records.

    URL parameter:
        sensor_id – integer ID of the sensor to delete

    Returns:
        200 – sensor successfully deleted
        404 – sensor not found
        409 – sensor has associated data (deletion blocked)
        500 – database error
    """

    conn = get_db()
    try:
        # Count how many MQTT_DATA rows reference this sensor.

        data_count = conn.execute("SELECT COUNT(*) FROM MQTT_DATA WHERE SENSOR_ID = ?", (sensor_id,)).fetchone()[0]
        if data_count > 0:
            return jsonify({"error": f"Cannot delete sensor with {data_count} data points. Delete data first."}), 409
        
        # Attempt to delete the sensor.
        cur = conn.execute("DELETE FROM SENSORS WHERE ID = ?", (sensor_id,))
        if cur.rowcount == 0:
            return jsonify({"error": "Sensor not found"}), 404
        
        conn.commit()
        return jsonify({"message": "Sensor deleted"})
    
    except sqlite3.Error as e:
        return jsonify({"error": str(e)}), 500
    
    finally:
        conn.close()

########################################################################
