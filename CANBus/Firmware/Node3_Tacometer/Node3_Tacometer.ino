/*
 * Arduino UNO R4 Minima - Write Example
 * Copyright (c) Sandeep Mistry. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for full license information.
 *
 * Based on CANWrite example from Arduino_CAN library
 * This reads pulses from a Tacometer, processes and writes data to the CAN Bus.
 *** Written by Electronic Cats team ***
 */

#include <Arduino_CAN.h>

static uint32_t const CAN_ID = 3ul; //Node ID for this device is 3
int pulses, revs, revsPerMin; //Speed elements
byte kmPerHour; //Speed element processed
const int incomingPulse = 3; //Interrupt pin for getting the pulses 
constexpr auto sendInterval {1000lu}; //Delay time between writings
auto sendNow {0lu}; //Used to refresh the delay time

//This is the interrupt that registers the incoming pulses from HC-020K sensor
void pulseISR() {
  pulses+=1;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { }
  pinMode(incomingPulse, INPUT); //Sensor signal
  attachInterrupt(digitalPinToInterrupt(incomingPulse), pulseISR, RISING); //Set the interruption for incoming pulses
  Serial.println("Node 3: Tacometer"); 
  pulses = 0;

  if (!CAN.begin(CanBitRate::BR_500k)) { // start the CAN bus at 500 kbps
    Serial.println("Starting node 3 failed!");
    while (1);
  }
}

void loop() {
  char kmsPerHour; //Final data to be transmitted
  if(millis() - sendInterval > sendNow){
    revs = pulses / 20; // Our gear has 20 teeth
    revsPerMin = revs * 60; // Here, we get the revolutions per minute
    kmPerHour = (revsPerMin*8*60*10)/100000; // The circumference of our gear wheel is 8cm, so we get the km/h. We multiply the result by 10 times.
    kmsPerHour = char(kmPerHour);
    const uint8_t dataToWrite[]={kmsPerHour}; //Buffer of data to be transmitted
    CanMsg msgOut(CAN_ID, sizeof(dataToWrite), dataToWrite); //CAN Frame completed
    int const rc = CAN.write(msgOut);
    Serial.print("Sending Tacometer: ");
    Serial.print(kmsPerHour, DEC); //Prints the data in decimal format
    if (rc <= 0) {
      Serial.print("CAN write failed with error code: ");
      Serial.println(rc);
      while(1) ;
    }
    Serial.println(" done");
    pulses = 0;
    sendNow = millis();
  }
}