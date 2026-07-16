import datetime
from arduino.app_bricks.web_ui import WebUI
from arduino.app_utils import App, Bridge

# Initialize the Local Web UI component to handle frontend data routing
ui = WebUI()

def record_sensor_samples(celsius: float, humidity: float):
    """Callback function invoked by the Arduino MCU via Bridge.
    Receives real-time sensor data and forwards it directly to the kiosk dashboard.
    """
    if celsius is None or humidity is None:
        print(f"Invalid samples received: celsius={celsius}, humidity={humidity}")
        return

    # Broadcast real-time values to the frontend using WebSockets
    ui.send_message('temperature', {"value": float(celsius)})
    ui.send_message('humidity', {"value": float(humidity)})

    # Local terminal monitoring for rapid debugging and verification
    print(f"[{datetime.datetime.now().strftime('%H:%M:%S')}] Temp: {celsius}°C | Hum: {humidity}%")

print("Registering RPC callback 'record_sensor_samples'...")
# Expose the Python function to the MCU environment
Bridge.provide("record_sensor_samples", record_sensor_samples)

print("Starting lightweight Kiosk Backend...")
# Run the underlying application server
App.run()