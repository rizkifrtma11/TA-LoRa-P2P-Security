/*
ATTACKER / SNIFFER + TAMPERING LoRa
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
Alat ini menyadap (Sniff) paket di udara, mengambil ID dan Timestamp aslinya,
kemudian memanipulasi (Tampering) nilai Suhu secara random dan mengubah Statusnya.
Setelah dimanipulasi, paket langsung ditembakkan kembali ke udara.
--------------------------------
*/
/*
ATTACKER / PASSIVE SNIFFER LoRa (AES/HMAC FORMAT)
-------------------------------------------------
Author: Mohammad Rizki Fadillah

Passive sniffing only:
- Tidak transmit
- Tidak replay
- Tidak tamper
- Hanya capture packet

Format output dibuat konsisten
dengan Receiver Baseline/Test.
-------------------------------------------------
*/

#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

// ===== PARAM =====
const uint32_t EXPECTED = 200;
const unsigned long TIMEOUT_MS = 40000;

// ===== METRICS =====
uint32_t sniffedCount   = 0;
uint32_t decodedCount   = 0;
uint32_t invalidCount   = 0;
uint32_t duplicateCount = 0;
uint32_t uniqueCount    = 0;

uint32_t totalBytes = 0;

unsigned long firstRecvTime = 0;
unsigned long lastRecvTime  = 0;

bool started = false;
bool done    = false;

// ===== TRACK UNIQUE =====
bool seen[EXPECTED] = {false};

// ===== VALIDASI PAYLOAD =====
bool isValidPayload(String data, uint32_t &id) {

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

  float pdr = (EXPECTED > 0)
              ? (uniqueCount * 100.0 / EXPECTED)
              : 0;

  float packetLoss = 100.0 - pdr;

  float durationSec =
      (lastRecvTime - firstRecvTime) / 1000.0;

  float throughput =
      (durationSec > 0)
      ? ((totalBytes * 8.0) / durationSec)
      : 0;

  float decodeRate =
      (sniffedCount > 0)
      ? ((decodedCount * 100.0) / sniffedCount)
      : 0;

  Serial.println("\n=== RESULT PASSIVE SNIFFER ===");

  Serial.print("Sniffed Total: ");
  Serial.println(sniffedCount);

  Serial.print("Decoded Valid: ");
  Serial.println(decodedCount);

  Serial.print("Invalid: ");
  Serial.println(invalidCount);

  Serial.print("Unique: ");
  Serial.println(uniqueCount);

  Serial.print("Duplicate: ");
  Serial.println(duplicateCount);

  Serial.print("PDR (%): ");
  Serial.println(pdr, 2);

  Serial.print("Packet Loss (%): ");
  Serial.println(packetLoss, 2);

  Serial.print("Decode Success (%): ");
  Serial.println(decodeRate, 2);

  Serial.print("Throughput (bps): ");
  Serial.println(throughput, 2);
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

  Serial.println("Passive Sniffer AES/HMAC Ready...");
}

void loop() {

  if (done) return;

  int packetSize = LoRa.parsePacket();

  // =========================================
  // RECEIVE PACKET
  // =========================================
  if (packetSize) {

    unsigned long tRecv = millis();

    if (!started) {
      firstRecvTime = tRecv;
      started = true;
    }

    lastRecvTime = tRecv;

    sniffedCount++;

    String data = "";

    while (LoRa.available()) {
      data += (char)LoRa.read();
    }

    totalBytes += data.length();

    uint32_t id = 0;

    bool valid = isValidPayload(data, id);

    if (valid) {

      decodedCount++;

      if (id < EXPECTED) {

        if (!seen[id]) {
          seen[id] = true;
          uniqueCount++;
        }
        else {
          duplicateCount++;
        }
      }
    }
    else {
      invalidCount++;
    }

    // =========================================
    // FORMAT LOG
    // =========================================
    Serial.print("[SNIFF] ");
    Serial.print(data);

    Serial.print(" | SIZE=");
    Serial.print(data.length());

    Serial.print(" | RSSI=");
    Serial.print(LoRa.packetRssi());

    Serial.print(" | SNR=");
    Serial.print(LoRa.packetSnr());

    if (!valid) {
      Serial.print(" | INVALID");
    }

    Serial.println();

    // =========================================
    // AUTO DONE
    // =========================================
    if (sniffedCount >= EXPECTED) {

      printSummary();

      done = true;
    }
  }

  // =========================================
  // TIMEOUT
  // =========================================
  if (started &&
      !done &&
      (millis() - lastRecvTime > TIMEOUT_MS)) {

    Serial.println(
      "\n[TIMEOUT] 40 detik tidak ada paket baru"
    );

    printSummary();

    done = true;
  }
}