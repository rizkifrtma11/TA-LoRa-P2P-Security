/*
RECEIVER BASELINE + ACK
--------------------------------
Author: Mohammad Rizki Fadillah

Description:
Receiver baseline LoRa
+ ACK response
+ metrik pengujian
*/

#include <SPI.h>
#include <LoRa.h>

#define SS    5
#define RST   14
#define DIO0  26

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);
  while (!Serial);

  SPI.begin(
    18,
    19,
    23,
    SS
  );

  LoRa.setPins(
    SS,
    RST,
    DIO0
  );

  if (
    !LoRa.begin(433E6)
  ) {

    Serial.println(
      "LoRa init FAILED!"
    );

    while (1);
  }

  LoRa.setSpreadingFactor(9);

  LoRa.setSignalBandwidth(
    125E3
  );

  LoRa.setCodingRate4(5);

  Serial.println(
    "================================"
  );

  Serial.println(
    "RECEIVER BASELINE READY"
  );

  Serial.println(
    "ACK ENABLED"
  );

  Serial.println(
    "Waiting packet..."
  );

  Serial.println(
    "================================"
  );

  LoRa.receive();
}

// =====================================
// LOOP
// =====================================
void loop() {

  int packetSize =
    LoRa.parsePacket();

  if (!packetSize)
    return;

  uint32_t startProc =
    micros();

  String payload =
    "";

  while (
    LoRa.available()
  ) {

    payload +=
      (char)
      LoRa.read();
  }

  // format:
  // packet_id,temp,hum,status
  int p1 =
    payload.indexOf(',');

  if (
    p1 == -1
  ) {

    LoRa.receive();
    return;
  }

  uint32_t packetId =
    payload.substring(
      0,
      p1
    ).toInt();

  // =====================================
  // SEND ACK
  // =====================================
  String ack =
      "ACK," +
      String(packetId);

  LoRa.beginPacket();

  LoRa.print(
    ack
  );

  LoRa.endPacket();

  LoRa.receive();

  // =====================================
  // METRICS
  // =====================================
  uint32_t procRx =
      micros()
      -
      startProc;

  int rssi =
    LoRa.packetRssi();

  float snr =
    LoRa.packetSnr();

  uint32_t freeRam =
      ESP.getFreeHeap();

  // =====================================
  // LOGGING
  // =====================================
  Serial.print(
    "[RX] id="
  );

  Serial.print(
    packetId
  );

  Serial.print(
    " | proc_rx="
  );

  Serial.print(
    procRx
  );

  Serial.print(
    "us | rssi="
  );

  Serial.print(
    rssi
  );

  Serial.print(" | packet_size=");
  Serial.print(packetSize);

  Serial.print(
    " | snr="
  );

  Serial.print(
    snr
  );

  Serial.print(
    " | ram="
  );

  Serial.print(
    freeRam
  );

  Serial.print(
    " | payload="
  );

  Serial.println(
    payload
  );
}