/*
 * Arduino Giga R1 - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANWrite example from Arduino_CAN library
 * This code writes data from a BME280 (Barometric and temperature sensor) to the CAN Bus.
 *** Written by Electronic Cats team ***
 */

#include <Arduino_CAN.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
static uint32_t const CAN_ID = 1ul; //Node ID for this device is 1
unsigned int status; //Status of BME Sensor
int temperature, pressure; //Data obtained from BME sensor
char temperatureSent, pressureDig1, pressureDig2, pressureDig3; //Processed data
constexpr auto sendInterval {5000lu}; //Delay time between writings
auto sendNow {0lu}; //Used to refresh the delay time

void setup() {
  Serial.begin(9600);
  Serial.println("Node 1: Barometric & temperature sensor"); 
  status = bme.begin();  //Initialize the BME Sensor
  if (!status) { //If BME sensor failed to initialize:
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1);
  }

  if (!CAN.begin(CanBitRate::BR_500k)) { // If the CAN bus at 500 kbps failed to start
    Serial.println("Starting node 1 failed!");
    while (1);
  }
}

void loop() {
  if(millis() - sendInterval > sendNow){
    temperature = bme.readTemperature(); //Read termperature from sensor
    pressure = bme.readPressure() / 100.0; //Read pressure from sensor

    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.print("Pressure = ");
    Serial.print(pressure);
    Serial.println(" hPa");

    temperatureSent = char(temperature); //First position of the array
    pressureDig1 = char((pressure / 100)+48); //Gets the first digit and converts to char, second position of the array
    pressureDig2 = char(((pressure % 100) / 10)+48); //Gets the second digit and converts to char second third of the array
    pressureDig3 = char((pressure % 10)+48); //Gets the third digit and converts to char, fourth position of the array
    const uint8_t dataToWrite[]={temperatureSent, pressureDig1, pressureDig2, pressureDig3}; //Array to be sent
    CanMsg msgOut(CAN_ID, sizeof(dataToWrite), dataToWrite);
    int const rc = CAN.write(msgOut);
    Serial.print("Sending data: ");
    if (rc <= 0) { //If something went wrong during the writing process
      Serial.print("CAN write failed with error code: ");
      Serial.println(rc);
      while(1) ;
    }
    Serial.println(" done!\n");
    sendNow = millis();
  }
}

