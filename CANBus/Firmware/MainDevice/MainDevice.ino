/*
 * Portenta Machine Control - CAN Read / Write Example
 * This example code is in the public domain. 
 * Copyright (c) 2024 Arduino
 * SPDX-License-Identifier: MPL-2.0
 *
 * Based on ReadCAN and WriteCAN examples from Arduino_PortentaMachineControl library.
 * This code reads a digital input (inductive sensor) from Portenta Machine Control and writes info to the CAN Bus.
 * This code reads a information from the CAN Bus to obtain data from a Tacometer, Barometric & temperature sensor.
 * This code writes information to the CAN Bus to control a motor driver and a lamp status.
 *** Written by Electronic Cats team ***
 */

#include <Arduino_PortentaMachineControl.h>

static uint32_t const CAN_ID = 37ul; //Node ID for this device is 37
constexpr auto receiveInterval {200lu}; //Delay time between readings
auto receiveNow {0lu}; //Used to refresh the delay time
byte incomingMsg[8], sizeMsg; //To manage the incomming information
int nodeNumber; //Node from which we are receiving information
byte controlMotor, controlLamp; //Aux for controlling actuators

void setup() {
  Serial.begin(9600);
  Wire.begin();
  if (!MachineControl_DigitalInputs.begin()) { // If the digital inputs failed to start
    Serial.println("Failed to initialize the digital input GPIO expander!");
  }
  if (!MachineControl_CANComm.begin(CanBitRate::BR_500k)) { // If the CAN bus at 500 kbps failed to start
    Serial.println("CAN Main init failed.");
    while(1) ;
  }
}

void loop() {
  if (MachineControl_CANComm.available()) {
    CanMsg const msgIn = MachineControl_CANComm.read();
    nodeNumber = int(msgIn.id);
    sizeMsg = byte(msgIn.data_length);
    for (int i=0; i <= sizeMsg - 1; i++) {
      incomingMsg[i]=byte(msgIn.data[i]);
    }
    switch(nodeNumber){
      case 1: 
        Serial.println("Barometric & temperature: ");
        for (int bt = 0; bt <= sizeMsg - 1; bt++){
          if (bt < 1){
            Serial.print(incomingMsg[bt]);
            Serial.println("° C");
          }
          else{
            Serial.print(char(incomingMsg[bt]));
          }
        }
        Serial.println("hPA");
      break;
      case 3: 
        Serial.println("Tacometer: ");
        for (int t = 0; t <= sizeMsg - 1; t++){
          if (incomingMsg[t] <= 25){
            controlMotor = 61; //ASCII "a" to accelerate
          }
          else{
            controlMotor = 64; //ASCII "d" to descelerate
          }
          Serial.print(incomingMsg[t]);
        }
        Serial.println(" km/h");
      break;
      default:
        Serial.print("No data received");
      break;
    }
  }
  if (millis() - receiveInterval > receiveNow){
    controlLamp = MachineControl_DigitalInputs.read(DIN_READ_CH_PIN_00);
    Serial.println(controlLamp);
    const uint8_t dataToWrite[] = {controlLamp, controlMotor};
    CanMsg msgOut(CAN_ID, sizeof(dataToWrite), dataToWrite);
    int const rc = MachineControl_CANComm.write(msgOut);
    if (rc <= 0) { //If something went wrong during the writing process
      Serial.print("CAN write failed with error code: ");
      Serial.println(rc);
      while(1) ;
    }
    Serial.println("CAN write message!");
    receiveNow = millis();
  }
}