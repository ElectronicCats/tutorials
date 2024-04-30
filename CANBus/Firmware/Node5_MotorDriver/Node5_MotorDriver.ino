/*
 * Arduino DUE - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANReceiver example from CAN library
 * This code reads data from the CAN Bus and controls the RPMs for a DC motor
 *** Written by Electronic Cats team ***
 */

#include <CAN.h>

constexpr auto sendInterval {1000lu};
auto sendNow {0lu};
int motorDriver = 2;
int speed = 0;  
int slowDown = 20;  

void setup() {
  Serial.begin(9600);
  Serial.println("Node 7: Motor driver");

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
}

void loop() {
  // send packet: id is 11 bits, packet can contain up to 8 bytes of data
  int packetSize = CAN.parsePacket();
  int speedCommand;
  if (packetSize || CAN.packetId() != -1) {
    // received a packet
    if (CAN.packetId() == 37){
      while (CAN.available()) {
        speedCommand = CAN.read();
        analogWrite(motorDriver, speed); 
        if (speedCommand == 65){
          if (speed >= 100) {
            speed = speed - slowDown;
          }
          delay(100);
        }
        if (speedCommand == 61){
          if (speed <= 255) {
            speed = speed + slowDown;
            delay(100);
          }
        }
      }  
    }
  }
  Serial.println(speed);
}

