#include <Arduino_Modulino.h>
#include <Arduino_RouterBridge.h>

// Create object instance for the Modulino Thermo sensor
ModulinoThermo thermo;

unsigned long previousMillis = 0;   // Stores the last timestamp data was captured
const long interval = 1000;         // Sampling interval set to 1 second (1000ms)

void setup() {
  // Initialize the inter-processor communication bridge (MCU <-> MPU)
  Bridge.begin();

  // Initialize Modulino I2C communication interface
  Modulino.begin(Wire1);
  
  // Detect and establish connection with the temperature/humidity module
  thermo.begin();
}

void loop() {
  unsigned long currentMillis = millis(); // Get current uptime execution time
  
  // Non-blocking timer check
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Fetch floating-point environmental metrics from the hardware sensor
    float celsius = thermo.getTemperature();
    float humidity = thermo.getHumidity();

    // Notify the MPU Python script over the bridge with the fresh telemetry
    Bridge.notify("record_sensor_samples", celsius, humidity);
  }
}