#include <SPI.h>
#include <LoRa.h>

int spreadFactor = 8;
int bwReference = 7;
int codingRate = 5;
byte syncWord = 0x12;
int preambleLength = 8;
int txPower = 17;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  pinMode(RFM_SWITCH,OUTPUT);
  // RF switch 1 to Rx
  digitalWrite(RFM_SWITCH,1);  
  Serial.println("LoRa Receiver");
  LoRa.setPins(SS, RFM_RST, RFM_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

 LoRa.setFrequency(433.33E6); //Set here the LoRa frequency
  LoRa.setSpreadingFactor(spreadFactor);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(codingRate);
  LoRa.setSyncWord(0x12);
  LoRa.setPreambleLength(8);
  //LoRa.enableInvertIQ();

}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}