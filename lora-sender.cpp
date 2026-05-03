/*
SENDER
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code is for sending data using LoRa communication protocol with an ESP32 microcontroller.
It initializes the LoRa module, sets the necessary parameters, and sends a message every second
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

#define SS   5
#define RST  14
#define DIO0 26

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// ===== PARAM UJI =====
const uint32_t NUM_PACKETS = 100;
const uint32_t INTERVAL_MS = 5000;

// ===== STATE =====
uint32_t counter = 0;
unsigned long lastSend = 0;
bool testDone = false;

// ===== METRIK =====
uint32_t totalPayloadBytes = 0;
unsigned long totalProcTime = 0;
uint32_t totalRamUsed = 0;

void setup() {
  Serial.begin(115200);

  // KASIH JEDA BIAR DAYA STABIL & DHT PEMANASAN
  Serial.println("Booting... Tunggu 2 detik");
  delay(2000); 

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433000000)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);

  dht.begin();

  Serial.println("Sender Baseline (High-Res Metrics) Ready. Mulai ngirim...");
}

void loop() {
  if (testDone) return;

  if (millis() - lastSend >= INTERVAL_MS) {
    lastSend = millis();

    float temp = dht.readTemperature();

    // ===== HANDLE ERROR SENSOR =====
    if (isnan(temp)) {
      Serial.println("DHT error, pakai dummy value");
      temp = -99.0;
    }

    String status = (temp > 30) ? "PANAS" : "AMAN";
    unsigned long tSend = millis();

    // ==========================================
    // ===== BACA RAM & WAKTU MULAI (CPU) =====
    // ==========================================
    uint32_t ramSebelum = ESP.getFreeHeap();
    unsigned long tStart = micros();

    // ---> (Nanti KDF, AES, sama HMAC masuk sini) <---

    String payload = String(counter) + "," +
                     String(tSend) + "," +
                     String(temp, 2) + "," +
                     status;

    // ==========================================
    // ===== BACA RAM & WAKTU SELESAI (CPU) =====
    // ==========================================
    unsigned long tEnd = micros();
    uint32_t ramSesudah = ESP.getFreeHeap();

    // Kalkulasi beban komputasi dan memori
    unsigned long procTime = (tEnd > tStart) ? (tEnd - tStart) : 0; // mikrodetik (us)
    uint32_t ramTerpakai = (ramSebelum > ramSesudah) ? (ramSebelum - ramSesudah) : 0; // Bytes

    totalProcTime += procTime;
    totalPayloadBytes += payload.length();
    totalRamUsed += ramTerpakai;

    // ===== SEND =====
    LoRa.beginPacket();
    LoRa.print(payload);
    LoRa.endPacket();

    // ===== LOG =====
    Serial.print("SEND: ");
    Serial.print(payload);
    Serial.print(" | SIZE=");
    Serial.print(payload.length());
    Serial.print(" | PROC_TX=");
    Serial.print(procTime);
    Serial.print(" us | RAM_USED=");
    Serial.print(ramTerpakai);
    Serial.println(" Bytes");

    counter++;

    // ===== DONE =====
    if (counter >= NUM_PACKETS) {
      Serial.println("\n=== SENDER RESULT ===");

      float avgProc = totalProcTime / (float)NUM_PACKETS;
      float avgRam = totalRamUsed / (float)NUM_PACKETS;

      Serial.print("Total Packet: "); Serial.println(NUM_PACKETS);
      Serial.print("Total Payload Bytes: "); Serial.println(totalPayloadBytes);
      Serial.print("Avg Processing Time TX (us): "); Serial.println(avgProc);
      Serial.print("Avg RAM Used (Bytes): "); Serial.println(avgRam);

      testDone = true;
    }
  }
}