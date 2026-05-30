########################################################################
# MQTT broker connection settings.

# Replace the placeholder values with your actual broker details.

# Hostname or IP address of the MQTT broker (e.g., "192.168.1.100" or
# "mqtt.example.com").
MQTT_SERVER     = "mqtt_server_ip"

# TCP port on which the broker is listening (default is 1883 for
# non‑TLS connections).
MQTT_PORT       = 1883

# Username for authenticating with the MQTT broker (if required).
MQTT_USER_NAME  = "mqtt_user_name"

# Password for the given username.
MQTT_PASSWORD   = "mqtt_password"

# Unique client identifier sent to the broker. Must be unique per
# connected client.
MQTT_CLIENT_ID  = "greenhouse-web"

# Quality of Service level used when subscribing to topics:
# 0 – at most once delivery (no guarantee)
# 1 – at least once delivery (default, ensures message is received)
# 2 – exactly once delivery (highest guarantee, more overhead)
MQTT_QOS        = 1

########################################################################
