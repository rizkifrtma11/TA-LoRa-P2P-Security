/*
RECEIVER CODE
--------------------------------
Author: Mohammad Rizki  Fadillah

Deskripsi:
Kode ini merupakan contoh penerima (receiver) untuk komunikasi LoRa.
Kode ini akan menerima paket data yang dikirim oleh perangkat lain melalui LoRa,
kemudian memproses payload yang diterima, menghitung metrik seperti latensi, RSSI, SNR,
waktu pemrosesan, dan sisa RAM, serta menampilkan informasi tersebut di serial monitor.
Lalu menyimpan data di database untuk analisis lebih lanjut.
--------------------------------
*/
#include <SPI.h>
#include <LoRa.h>

#define SS    5
#define RST   14
#define DIO0  2

void setup() {

  Serial.begin(115200);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa init failed!");

    while (1);
  }

  // =========================
  // LoRa Configuration
  // =========================
  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(125E3);

  Serial.println("Receiver Ready");

  LoRa.receive();
}

void loop() {

  uint32_t startProc = micros();

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String payload = "";

    while (LoRa.available()) {

      payload += (char)LoRa.read();
    }

    // =========================
    // Parse Payload
    // Format:
    // ID,TIMESTAMP,TEMP,HUM,STATUS
    // =========================

    int idx1 = payload.indexOf(',');
    int idx2 = payload.indexOf(',', idx1 + 1);

    long senderTimestamp =
      payload.substring(idx1 + 1, idx2).toInt();

    // =========================
    // Metrics
    // =========================

    long latency =
      millis() - senderTimestamp;

    int rssi = LoRa.packetRssi();

    float snr = LoRa.packetSnr();

    uint32_t procRx =
      micros() - startProc;

    uint32_t freeRam =
      ESP.getFreeHeap();

    // =========================
    // Serial Output
    // =========================

    Serial.print("PAYLOAD=");
    Serial.print(payload);

    Serial.print("|LAT=");
    Serial.print(latency);

    Serial.print("|RSSI=");
    Serial.print(rssi);

    Serial.print("|SNR=");
    Serial.print(snr);

    Serial.print("|PROC=");
    Serial.print(procRx);

    Serial.print("|SIZE=");
    Serial.print(packetSize);

    Serial.print("|RAM=");
    Serial.println(freeRam);

    LoRa.receive();
  }
}