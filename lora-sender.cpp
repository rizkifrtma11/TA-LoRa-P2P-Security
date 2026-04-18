/*
SENDER
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code is for sending data using LoRa communication protocol with an ESP32 microcontroller.
It initializes the LoRa module, sets the necessary parameters, and sends a message every second
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

  LoRa.setTxPower(17);

  Serial.println("LoRa ESP32 Ready");
}

void loop() {
  LoRa.beginPacket();
  LoRa.print("HIDUP JOKOWI");
  LoRa.endPacket();

  Serial.println("Kirim dari ESP32");
  delay(10000);
}
