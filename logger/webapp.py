import threading
import dash
import plotly.express as px
import pandas as pd
import sqlite3
import threading
import const

from dash import dcc, html
from dash.dependencies import Input, Output
from datetime import datetime
from datetime import datetime, timedelta
from collections import OrderedDict


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
        conn = sqlite3.connect(const.DATABASE_FILE_NAME)
        
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
            const.DATA_TYPE_AVG_TEMPERATURE:  "Average",
            const.DATA_TYPE_BATTERY_CAPACITY: "Capacity",
            const.DATA_TYPE_BATTERY_VOLTAGE:  "Voltage",
            const.DATA_TYPE_ADJUSTED_VOLATGE: "Adjusted",
        }
        
        # Add temperature sensors
        for i in range(const.SENSOR_ZONES_COUNT):
            type_mapping[const.DATA_TYPE_TEMPERATURE_SENSOR + i] = f"Zone {i}"
        
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
        temp_df = df[df["Sensor"].str.contains("Average|Zone")]
        temp_fig = px.line(
            temp_df, 
            x       = "DATE_TIME", 
            y       = "VALUE", 
            color   = "Sensor",
            title   = f"Temperature Sensors - Last {time_label}",
            labels  = {"VALUE": "Temperature (°C)", "DATE_TIME": "Time"}
        )
        temp_fig.update_layout(
            legend = dict(
                orientation = "h",      # Horizontal legend
                yanchor     = "bottom", # Anchor to bottom
                y           = 1.02,     # Position above plot
                xanchor     = "right",  # Anchor to right
                x           = 1         # Align to right
            ),
            margin = dict(t = 50) # Add top margin to prevent overlap
        )
        
        # Create battery plot
        battery_df = df[df["Sensor"].str.contains("Capacity|Voltage|Adjusted")]
        battery_fig = px.line(
            battery_df, 
            x       = "DATE_TIME", 
            y       = "VALUE", 
            color   = "Sensor",
            title   = f"Battery Status - Last {time_label}",
            labels  = {"VALUE": "Value", "DATE_TIME": "Time"}
        )
        battery_fig.update_layout(
            legend = dict(
                orientation = "h",      # Horizontal legend
                yanchor     = "bottom", # Anchor to bottom
                y           = 1.02,     # Position above plot
                xanchor     = "right",  # Anchor to right
                x           = 1         # Align to right
            ),
            margin = dict(t = 50)  # Add top margin to prevent overlap
        )
        
        return temp_fig, battery_fig
    
    return app
########################################################################################


if __name__ == "__main__":
    app = create_dashboard_app()
    app.run(host = "0.0.0.0", port = 8228, debug = False)