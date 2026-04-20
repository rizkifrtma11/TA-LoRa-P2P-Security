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

long freqList[] = {
  433000000,
  433050000,
  433100000,
  433150000
};

int sfList[] = {7, 8, 9, 10, 11, 12};

int fIndex = 0;
int sfIndex = 0;

unsigned long lastSwitch = 0;
const unsigned long scanInterval = 2000;

bool locked = false;

// =======================
// APPLY CONFIG
// =======================
void applyConfig() {
  LoRa.idle();

  if (!LoRa.begin(freqList[fIndex])) {
    Serial.println("Freq init failed!");
    return;
  }

  LoRa.setSpreadingFactor(sfList[sfIndex]);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();

  Serial.print("Scan Freq: ");
  Serial.print(freqList[fIndex]);
  Serial.print(" | SF: ");
  Serial.println(sfList[sfIndex]);
}

// =======================
// VALIDASI PAYLOAD
// =======================
bool isValidPayload(String data) {
  int commaCount = 0;

  for (int i = 0; i < data.length(); i++) {
    if (data[i] == ',') commaCount++;
  }

  // format: counter,timestamp,temp,status → 3 koma
  if (commaCount >= 3 && data.length() > 10) {
    return true;
  }

  return false;
}

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(freqList[0])) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  applyConfig();

  Serial.println("Sniffer Scanner Ready...");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String received = "";

    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    // =========================
    // MODE SCANNING
    // =========================
    if (!locked) {
      Serial.print("[RAW] ");
      Serial.println(received);

      if (isValidPayload(received)) {
        Serial.println("✅ VALID PACKET FOUND!");

        Serial.print("LOCKED Freq=");
        Serial.print(freqList[fIndex]);
        Serial.print(" SF=");
        Serial.println(sfList[sfIndex]);

        Serial.print("DATA: ");
        Serial.println(received);

        locked = true;
      } else {
        Serial.println("❌ NOISE / INVALID");
      }
    }

    // =========================
    // MODE LOCKED (SNIFF ONLY)
    // =========================
    else {
      Serial.print("[SNIFF LOCKED] ");
      Serial.print(received);

      Serial.print(" | RSSI: ");
      Serial.print(LoRa.packetRssi());

      Serial.print(" | SNR: ");
      Serial.println(LoRa.packetSnr());
    }
  }

  // =========================
  // SCANNING ONLY IF NOT LOCKED
  // =========================
  if (!locked && millis() - lastSwitch > scanInterval) {
    lastSwitch = millis();

    sfIndex++;
    if (sfIndex >= sizeof(sfList)/sizeof(sfList[0])) {
      sfIndex = 0;
      fIndex++;
      if (fIndex >= sizeof(freqList)/sizeof(freqList[0])) {
        fIndex = 0;
      }
    }

    applyConfig();
  }
}