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

// ===== METRICS =====
uint32_t sniffedCount = 0;
uint32_t decodedCount = 0;
uint32_t invalidCount = 0;

// ===== OPTIONAL UNIQUE TRACK =====
bool seen[EXPECTED] = {false};
uint32_t uniqueCount = 0;
uint32_t duplicateCount = 0;

// ===== TIMEOUT LOGIC =====
unsigned long lastRecvTime = 0;
bool started = false;
bool done = false; // Flag biar summary cuma ke-print sekali

// ===== VALIDATION =====
bool isValidPayload(String data, uint32_t &id) {
  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  if (p1 == -1 || p2 == -1) return false;

  String idStr = data.substring(0, p1);

  // cek ID numeric
  for (int i = 0; i < idStr.length(); i++) {
    if (!isDigit(idStr[i])) return false;
  }

  id = idStr.toInt();
  return true;
}

void printSummary() {
  float sniffRate = (sniffedCount * 100.0) / EXPECTED;
  // Cegah pembagian dengan nol
  float decodeRate = (sniffedCount > 0) ? ((decodedCount * 100.0) / sniffedCount) : 0;

  Serial.println("\n=== SNIFFER SUMMARY ===");

  Serial.print("Sniffed Total: "); Serial.println(sniffedCount);
  Serial.print("Decoded Valid: "); Serial.println(decodedCount);
  Serial.print("Invalid: "); Serial.println(invalidCount);

  Serial.print("Unique: "); Serial.println(uniqueCount);
  Serial.print("Duplicate: "); Serial.println(duplicateCount);

  Serial.print("Sniff Success (%): "); Serial.println(sniffRate);
  Serial.print("Decode Success (%): "); Serial.println(decodeRate);
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

  Serial.println("Sniffer + Summary Ready (With Timeout)...");
}

void loop() {
  if (done) return; // Kalau udah selesai, stop proses

  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    // ===== UPDATE TIMER =====
    lastRecvTime = millis();
    started = true;

    sniffedCount++;

    String data = "";
    while (LoRa.available()) {
      data += (char)LoRa.read();
    }

    uint32_t id = 0;
    bool valid = isValidPayload(data, id);

    if (valid) {
      decodedCount++;

      // ===== UNIQUE / DUPLICATE =====
      if (id < EXPECTED) {
        if (!seen[id]) {
          seen[id] = true;
          uniqueCount++;
        } else {
          duplicateCount++;
        }
      }
    } else {
      invalidCount++;
    }

    // ===== LOG =====
    Serial.print("[SNIFF] ");
    Serial.print(data);

    Serial.print(" | RSSI=");
    Serial.print(LoRa.packetRssi());

    Serial.print(" | SNR=");
    Serial.print(LoRa.packetSnr());

    if (!valid) Serial.print(" | INVALID");

    Serial.println();

    // Kalau pas beruntung nangkep 200 tanpa loss
    if (sniffedCount >= EXPECTED) {
      printSummary();
      done = true;
    }
  }

  // ===== TIMEOUT LOGIC (50 Detik) =====
  if (started && !done && (millis() - lastRecvTime > 50000)) {
    Serial.println("\n[TIMEOUT] 50 Detik tidak ada paket baru. Sesi Sniffing dianggap selesai.");
    printSummary();
    done = true;
  }
}