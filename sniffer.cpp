/*
ATTACKER / SNIFFER LoRa
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code implements a LoRa sniffer that listens for LoRa packets on a specified frequency and prints the received data
along with signal strength (RSSI) and signal-to-noise ratio (SNR). It uses the LoRa library to interface with the LoRa
module and the SPI library for communication. The sniffer is configured to use specific pins for SS, RST, and DIO0, and
it sets the spreading factor, signal bandwidth, and coding rate to match the sender's configuration. The code continuously
checks for incoming packets and prints their contents when detected.

This code is for educational purposes only. Unauthorized use may be illegal and unethical.
Always obtain proper permissions before conducting any security testing.
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

  Serial.println("LoRa Attacker (Sniffer)");

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433000000)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  // HARUS SAMA dengan sender
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();

  Serial.println("Sniffer Ready...");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    Serial.print("[SNIFF] ");

    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    Serial.print(" | RSSI: ");
    Serial.print(LoRa.packetRssi());

    Serial.print(" | SNR: ");
    Serial.println(LoRa.packetSnr());
  }
}