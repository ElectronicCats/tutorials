/*
 * Arduino UNO R4 Minima - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANSender example from CAN library
 * This code writes data from a BME280 (Barometric and temperature sensor) to the CAN Bus.
 *** Written by Electronic Cats team ***
 */

#include <CAN.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;
unsigned int status;
constexpr auto sendInterval {1000lu};
auto sendNow {0lu};

void setup() {
  Serial.begin(9600);
  unsigned int status; //Status of BME Sensor
  Serial.println("Node 1: Barometric & temperature sensor"); 
  status = bme.begin();  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1) delay(10);
  }

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting node 1 failed!");
    while (1);
  }
}

void loop() {
  // send packet: id is 11 bits, packet can contain up to 8 bytes of data
  if(millis() - sendInterval > sendNow){
    int temperature = bme.readTemperature();
    int pressure = bme.readPressure() / 100.0;
    char temperatureSent, pressureDig1, pressureDig2, pressureDig3;

    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" °C");
    Serial.print("Pressure = ");
    Serial.print(pressure);
    Serial.println(" hPa");

    temperatureSent = char(temperature);
    pressureDig1 = char((pressure / 100)+48);
    pressureDig2 = char(((pressure % 100) / 10)+48);
    pressureDig3 = char((pressure % 10)+48);
    uint8_t const msg[]={temperatureSent, pressureDig1, pressureDig2, pressureDig3};
    CAN.beginPacket(1);
    CAN.write(msg, sizeof(msg));
    CAN.endPacket();
    Serial.println("Send data done!");
    sendNow = millis();
  }
}

