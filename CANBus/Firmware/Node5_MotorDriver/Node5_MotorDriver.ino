/*
 * Arduino UNO WiFi - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANRead example from Arduino_CAN library
 * This code reads data from the CAN Bus and controls the RPMs for a DC motor
 *** Written by Electronic Cats team ***
 */

#include <Arduino_CAN.h>

int motorDriver = 3, enablePin = 4; //Pins for L298N motor driver module
int speed = 50; //Initial speed
int accelerate = 20; //Acceleration rate
int nodeNumber; //Sender's node number
byte incomingMsg[8], sizeMsg; //CAN buffer information

void setup() {
  Serial.begin(9600);
  while (!Serial) { }
  pinMode(enablePin, OUTPUT); //Motor-driver "enabled" signal
  Serial.println("Node 7: Motor driver");

  if (!CAN.begin(CanBitRate::BR_500k)) { // If the CAN bus at 500 kbps failed to start
    Serial.println("Starting CAN failed!");
    while (1);
  }
  digitalWrite(enablePin, HIGH); //To enable motor-driver
}

void loop() {
  int speedCommand; //PWM for motor-driver to modify the current speed
  if (CAN.available()) {
    CanMsg const msgIn = CAN.read(); //Buffer received from CAN bus
    nodeNumber = int(msgIn.id);
    sizeMsg = byte(msgIn.data_length); //Size of the received message
    for (int i=0; i <= sizeMsg - 1; i++) {
      incomingMsg[i]=byte(msgIn.data[i]); //Copy every byte received into a new array
    }
    if(nodeNumber == 37){ //If the data received comes from the Main Device (node 37) do...
      for (int sc = 0; sc <= sizeMsg - 1; sc++){
        if (sc > 0){
          speedCommand = incomingMsg[sc]; //Read the speed command: accelerate or descelerate
        }
      }
    }
    analogWrite(motorDriver, speed); //write the PWM to the motor-driver
    if (speedCommand == 64){ //Descelerate
      if (speed >= 100) {
        speed = speed - accelerate;
      }
      delay(100);
    }
    if (speedCommand == 61){ //Accelerate
      if (speed <= 255) {
        speed = speed + accelerate;
        delay(100);
      }
    }
  }
}

