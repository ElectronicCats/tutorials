/*
 * Arduino Giga R1 - Read Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANRead example from Arduino_CAN library
 * This code reads data from the CAN Bus and activates a digital output
 *** Written by Electronic Cats team ***
 */

#include <Arduino_CAN.h>

int relayLamp = 1; //Output to the relay to turn-on a 12V DC lamp
int nodeNumber; //Sender's node number
byte incomingMsg[8], sizeMsg; //CAN buffer information
constexpr auto receiveInterval {200lu}; //Delay time between readings
auto receiveNow {0lu}; //Used to refresh the delay time

void setup() {
  Serial.begin(9600);
  while (!Serial) {};
  pinMode(relayLamp, OUTPUT); //Relay "enabled" signal
  Serial.println("Node 7: Inductive sensor state receiver");

  if (!CAN.begin(CanBitRate::BR_500k)) { // If the CAN bus at 500 kbps failed to start
    Serial.println("Starting node 7 failed!");
    while (1);
  }
}

void loop() {
  int biState; //Variable to save the status (ON/OFF) commanded in the buffer
  if (millis() - receiveInterval > receiveNow){
    if (CAN.available())
    {
      CanMsg const msgIn = CAN.read(); //Buffer received from CAN bus
      Serial.println(msgIn);
      nodeNumber = int(msgIn.id);
      sizeMsg = byte(msgIn.data_length); //Size of the received message
      for (int i=0; i <= sizeMsg - 1; i++) {
        incomingMsg[i]=byte(msgIn.data[i]); //Copy every byte received into a new array
      }
      if(nodeNumber == 37){ //If the data received comes from the Main Device (node DEC 37, HEX 25) do...
        biState = incomingMsg[0];
      }
      if (biState == 0x00){ //Turn on (inverse logic)
        digitalWrite(relayLamp, LOW);
      }
      if (biState == 0x01){ // Turn off (inverse logic)
        digitalWrite(relayLamp, HIGH);
      }
    }
    receiveNow = millis();
  }
}