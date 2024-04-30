/*
 * Arduino DUE - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANReceiver example from CAN library
 * This code reads data from the CAN Bus and activates a digital output
 *** Written by Electronic Cats team ***
 */

#include <CAN.h>

constexpr auto sendInterval {1000lu};
auto sendNow {0lu}; 
int relayLamp = 1;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  pinMode(relayLamp, OUTPUT);
  Serial.println("Node 7: Inductive sensor state receiver");

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting node 7 failed!");
    while (1);
  }
}

void loop() {
  // try to parse packet
  int packetSize = CAN.parsePacket();
  int biState;
  if (packetSize || CAN.packetId() != -1) {
    if (CAN.packetId() == 37){
      while (CAN.available()) {
      biState = CAN.read();
      if (biState == 10){
        digitalWrite(relayLamp, HIGH);
      }
      if (biState == 5){
        digitalWrite(relayLamp, LOW);
      }
      Serial.println(biState);
      }
    }
  }
}