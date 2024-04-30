/*
 * Arduino UNO R4 WiFi - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANSender example from CAN library
 * This code writes data from a Tacometer to the CAN Bus.
 *** Written by Electronic Cats team ***
 */

#include <Arduino.h>
#include <CAN.h>

int pulses, revs, revsPerMin;
byte kmPerHour;
const int incomingPulse = 1;
constexpr auto sendInterval {1000lu};
auto sendNow {0lu};

void pulseISR() {
  pulses+=1;
}

void setup() {
  pinMode(incomingPulse, INPUT);
  attachInterrupt(digitalPinToInterrupt(incomingPulse), pulseISR, RISING);
  Serial.begin(9600);
  Serial.println("Node 3: Tacometer");
  pulses = 0;

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting node 3 failed!");
    while (1);
  }
}

void loop() {
  // send packet: id is 11 bits, packet can contain up to 8 bytes of data
  char kmsPerHour;
  if(millis() - sendInterval > sendNow){
    revs = pulses / 20; // Our gear has 20 teeth
    revsPerMin = revs * 60; // Here, we get the revolutions per minute
    kmPerHour = (revsPerMin*8*60*10)/100000; // The circumference of our gear wheel is 8cm, so we get the km/h. We multiply the result by 10 times.
    kmsPerHour = char(kmPerHour);
    uint8_t const msg[]={kmsPerHour};
    CAN.beginPacket(3);
    CAN.write(msg, sizeof(msg));
    CAN.endPacket();
    Serial.print("Send Tacometer: ");
    Serial.print(kmsPerHour, DEC);
    Serial.println(" done");
    pulses = 0;
    sendNow = millis();
  }
}