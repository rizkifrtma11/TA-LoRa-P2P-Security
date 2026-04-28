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
const uint32_t NUM_PACKETS = 200;
const uint32_t INTERVAL_MS = 8000;

// ===== STATE =====
uint32_t counter = 0;
unsigned long lastSend = 0;
bool testDone = false;

// ===== METRIK =====
uint32_t totalPayloadBytes = 0;

// ===== PROCESSING TIME (placeholder) =====
unsigned long totalProcTime = 0;

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
  LoRa.setTxPower(17);

  dht.begin();

  Serial.println("Sender Baseline Ready");
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

    // ===== PROCESSING TIME START =====
    unsigned long tStart = millis();

    // (placeholder crypto - nanti AES + HMAC di sini)

    String payload = String(counter) + "," +
                     String(tSend) + "," +
                     String(temp, 2) + "," +
                     status;

    unsigned long tEnd = millis();
    unsigned long procTime = tEnd - tStart;

    totalProcTime += procTime;
    totalPayloadBytes += payload.length();

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
    Serial.println(" ms");

    counter++;

    // ===== DONE =====
    if (counter >= NUM_PACKETS) {
      Serial.println("\n=== SENDER RESULT ===");

      float avgProc = totalProcTime / (float)NUM_PACKETS;

      Serial.print("Total Packet: "); Serial.println(NUM_PACKETS);
      Serial.print("Total Payload Bytes: "); Serial.println(totalPayloadBytes);
      Serial.print("Avg Processing Time TX (ms): "); Serial.println(avgProc);

      testDone = true;
    }
  }
}