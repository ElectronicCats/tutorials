/*
 * Arduino UNO R4 WiFi - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANSender example from CAN library
 * This code writes data from a Tacometer to the CAN Bus.
 *** Written by Electronic Cats team ***
 */

#include <CAN.h>

byte pulses;
constexpr auto sendInterval {1000lu};
auto sendNow {0lu};

void setup() {
  Serial.begin(9600);
  pulses = 0;
  Serial.println("Node 3: Tacometer");

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting node 3 failed!");
    while (1);
  }
}

void loop() {
  // send packet: id is 11 bits, packet can contain up to 8 bytes of data
  char pulsesTacometer;
  if(millis() - sendInterval > sendNow){
    if (pulses>=255){
      pulses=0;
    }
     else{
      pulses+=1;
     }
    pulsesTacometer = char(pulses);
    uint8_t const msg[]={pulsesTacometer};
    CAN.beginPacket(3);
    CAN.write(msg, sizeof(msg));
    CAN.endPacket();
    Serial.print("Send Tacometer: ");
    Serial.print(pulsesTacometer, DEC);
    Serial.println(" done");
    sendNow = millis();
  }
}

