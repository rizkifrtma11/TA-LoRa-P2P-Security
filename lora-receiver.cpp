/*
RECEIVER
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code is for receiving data using LoRa on an ESP32. It initializes the LoRa module, sets the necessary parameters,
and continuously listens for incoming packets. When a packet is received, it prints the contents to the Serial Monitor.

Note: Make sure to connect the LoRa module to the correct pins (SS, RST, DIO0) as defined in the code.
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("LoRa ESP32 Starting...");

  SPI.begin(18, 19, 23, SS); // SCK, MISO, MOSI, SS
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433000000)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("LoRa ESP32 Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    Serial.print("Received: ");

    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    Serial.println();
  }
}