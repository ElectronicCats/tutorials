/*
  WiFi Modbus TCP Server LED

  This sketch creates a Modbus TCP Server for a Opta device to share the status of the inputs
  and update the outputs depending on the client requests.

*/

#include <SPI.h>
#include <WiFi.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS; // connection status

WiFiServer wifiServer(502);
ModbusTCPServer modbusTCPServer;

const int OUTP_LEDS[] = {LED_D0, LED_D1};
const int RELAYS[] = {D0, D1};
const int DigInputs[] = {A0, A1};
const int AnaInput = A2;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  /*while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/
  Serial.println("Modbus TCP Server Opta");
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID:");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();
  // start the server
  wifiServer.begin();
  // start the Modbus TCP server
  if (!modbusTCPServer.begin()) {
    Serial.println("Failed to start Modbus TCP Server!");
    while (1);
  }

  // configuring inputs and outputs
  analogReadResolution(12); // 0 - 4095 for 0 - 10V
  pinMode(LED_USER, OUTPUT);
  digitalWrite(LED_USER, LOW);
  for (int i=0; i<=1; i++){
    pinMode(OUTP_LEDS[i], OUTPUT);
    digitalWrite(OUTP_LEDS[i], LOW);
    pinMode(RELAYS[i], OUTPUT);
    digitalWrite(RELAYS[i], LOW);
    pinMode(DigInputs[i], INPUT);
  }
  // configure three coils starting at address 0x00
  modbusTCPServer.configureCoils(0x00, 4);
  modbusTCPServer.configureInputRegisters(0x04, 1);
}

void loop() {
  // listen for incoming clients
  WiFiClient client = wifiServer.available();
  if (client) {
    // a new client connected
    //Serial.println("new client");
    // let the Modbus TCP accept the connection 
    modbusTCPServer.accept(client);
    digitalWrite(LED_USER, HIGH);
    while (client.connected()) {
      // poll for Modbus TCP requests, while client connected
      modbusTCPServer.poll();
      // update inputs and outputs
      updateOuts();
      updateWInputs();
    }
    //Serial.println("client disconnected");
    digitalWrite(LED_USER, LOW);
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void updateOuts() {
  // Read current values of the coils matching inputs coming from HMI
  for (int i=0; i<=1; i++){
    int coilValue[] = {modbusTCPServer.coilRead(i)};
    if (coilValue[i]){
      digitalWrite(OUTP_LEDS[i], HIGH);
      digitalWrite(RELAYS[i], HIGH);
    }
    else
    {
      digitalWrite(OUTP_LEDS[i], LOW);
      digitalWrite(RELAYS[i], LOW);
    }
  }
}

void updateWInputs(){
  int voltage;
  for (int i=0; i<=1; i++){
    int j = i + 2;
    if (digitalRead(DigInputs[i])){
      modbusTCPServer.coilWrite(j, 1);
    }
    else{
      modbusTCPServer.coilWrite(j, 0);
    }
  }
  voltage = analogRead(AnaInput);
  Serial.println(voltage);
  modbusTCPServer.inputRegisterWrite(0x04, voltage);
}
