/*
RECEIVER
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code is for receiving data using LoRa on an ESP32. It initializes the LoRa module, sets the necessary parameters, and continuously listens for incoming packets. When a packet is received, it prints the contents to the Serial Monitor.
Note: Make sure to connect the LoRa module to the correct pins (SS, RST, DIO0) as defined in the code.
--------------------------------
*/
#include <SPI.h>
#include <LoRa.h>

#define SS   5
#define RST  14
#define DIO0 26

// ===== PARAM =====
const uint32_t NUM_PACKETS = 200;

// ===== METRIK =====
uint32_t receivedCount = 0;
uint32_t uniqueReceived = 0;
uint32_t duplicateCount = 0;
uint32_t invalidCount = 0;
uint32_t rejectedCount = 0; // nanti untuk HMAC/replay

uint32_t totalBytes = 0;

unsigned long firstRecvTime = 0;
unsigned long lastRecvTime  = 0;

unsigned long totalLatency = 0;
unsigned long minLatency = 999999;
unsigned long maxLatency = 0;

unsigned long totalProcTime = 0;

bool started = false;

// ===== TRACK ID =====
bool receivedFlags[NUM_PACKETS] = {false};

// ===== PARSE =====
bool parsePayload(String data, uint32_t &id, unsigned long &tSend) {
  int p1 = data.indexOf(',');
  int p2 = data.indexOf(',', p1 + 1);

  if (p1 == -1 || p2 == -1) return false;

  id = data.substring(0, p1).toInt();
  tSend = data.substring(p1 + 1, p2).toInt();

  return true;
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

  Serial.println("Receiver Baseline Ready (Full Metrics)");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String payload = "";

    while (LoRa.available()) {
      payload += (char)LoRa.read();
    }

    unsigned long tRecv = millis();

    // ===== PROCESSING TIME START =====
    unsigned long tStart = millis();

    uint32_t id;
    unsigned long tSend;

    if (!parsePayload(payload, id, tSend)) {
      invalidCount++;
      Serial.println("[INVALID PAYLOAD]");
      return;
    }

    unsigned long tEnd = millis();
    unsigned long procTime = tEnd - tStart;
    totalProcTime += procTime;

    unsigned long latency = tRecv - tSend;

    // ===== START TIME =====
    if (!started) {
      firstRecvTime = tRecv;
      started = true;
    }

    lastRecvTime = tRecv;
    receivedCount++;
    totalBytes += payload.length();
    totalLatency += latency;

    if (latency < minLatency) minLatency = latency;
    if (latency > maxLatency) maxLatency = latency;

    // ===== TRACK UNIQUE / DUPLICATE =====
    if (id < NUM_PACKETS) {
      if (!receivedFlags[id]) {
        receivedFlags[id] = true;
        uniqueReceived++;
      } else {
        duplicateCount++;
      }
    }

    // ===== LOG =====
    Serial.print("ID=");
    Serial.print(id);
    Serial.print(" | LAT=");
    Serial.print(latency);
    Serial.print(" ms | SIZE=");
    Serial.print(payload.length());
    Serial.print(" | RSSI=");
    Serial.print(LoRa.packetRssi());
    Serial.print(" | SNR=");
    Serial.print(LoRa.packetSnr());
    Serial.println();

    // ===== DONE =====
    if (receivedCount >= NUM_PACKETS) {
      Serial.println("\n=== RESULT ===");

      float durationSec = (lastRecvTime - firstRecvTime) / 1000.0;
      float pdr = (uniqueReceived * 100.0) / NUM_PACKETS;
      float packetLoss = 100.0 - pdr;
      float avgLatency = totalLatency / (float)receivedCount;
      float throughput = (totalBytes * 8.0) / durationSec;
      float avgProc = totalProcTime / (float)receivedCount;

      Serial.print("Total Received: "); Serial.println(receivedCount);
      Serial.print("Unique Received: "); Serial.println(uniqueReceived);
      Serial.print("Duplicate: "); Serial.println(duplicateCount);
      Serial.print("Invalid: "); Serial.println(invalidCount);
      Serial.print("Rejected (future): "); Serial.println(rejectedCount);

      Serial.print("PDR (%): "); Serial.println(pdr);
      Serial.print("Packet Loss (%): "); Serial.println(packetLoss);

      Serial.print("Avg Latency (ms): "); Serial.println(avgLatency);
      Serial.print("Min Latency (ms): "); Serial.println(minLatency);
      Serial.print("Max Latency (ms): "); Serial.println(maxLatency);

      Serial.print("Throughput (bps): "); Serial.println(throughput);
      Serial.print("Avg Processing Time RX (ms): "); Serial.println(avgProc);

      while (1);
    }
  }
}