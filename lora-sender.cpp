/*
SENDER BASELINE + ACK + DHT22
--------------------------------
Author: Mohammad Rizki Fadillah

Description:
Baseline LoRa tanpa keamanan
+ ACK RTT Latency
*/

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

#define SS    5
#define RST   14
#define DIO0  26

#define DHTPIN   4
#define DHTTYPE  DHT22

DHT dht(DHTPIN, DHTTYPE);

// =====================================
// CONFIG
// =====================================
const unsigned long ACK_TIMEOUT = 3000;

uint32_t packetId = 0;

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);
  while (!Serial);

  dht.begin();

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

  if (!LoRa.begin(433E6)) {

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

  LoRa.setTxPower(17);

  Serial.println(
    "================================"
  );

  Serial.println(
    "SENDER BASELINE READY"
  );

  Serial.println(
    "ACK RTT ENABLED"
  );

  Serial.println(
    "WAITING 5 SECONDS..."
  );

  Serial.println(
    "================================"
  );

  delay(5000);
}

// =====================================
// LOOP
// =====================================
void loop() {

  float t =
    dht.readTemperature();

  float h =
    dht.readHumidity();

  if (
    isnan(t) ||
    isnan(h)
  ) {

    Serial.println(
      "Gagal baca DHT22"
    );

    delay(2000);
    return;
  }

  // =====================================
  // CPU PROCESS START
  // =====================================
  uint32_t startProc =
    micros();

  packetId++;

  String status_data =
      (t > 30.0)
      ? "PANAS"
      : "NORMAL";

  // payload:
  // packet_id,temp,hum,status
  String payload =
      String(packetId)
      + "," +
      String(t, 1)
      + "," +
      String(h, 1)
      + "," +
      status_data;

  // =====================================
  // SEND PACKET
  // =====================================
  uint32_t t0 =
    millis();

  LoRa.beginPacket();

  LoRa.print(
    payload
  );

  LoRa.endPacket();

  // pindah RX mode
  LoRa.receive();

  // =====================================
  // WAIT ACK
  // =====================================
  bool ackReceived =
    false;

  uint32_t rtt = 0;

  uint32_t latency = 0;

  while (
    millis() - t0
    <
    ACK_TIMEOUT
  ) {

    int packetSize =
      LoRa.parsePacket();

    if (
      packetSize
    ) {

      String ack =
        "";

      while (
        LoRa.available()
      ) {

        ack +=
          (char)
          LoRa.read();
      }

      String expectedAck =
          "ACK," +
          String(packetId);

      if (
        ack ==
        expectedAck
      ) {

        ackReceived =
          true;

        rtt =
          millis()
          - t0;

        latency =
          rtt / 2;

        break;
      }
    }
  }

  // =====================================
  // METRICS
  // =====================================
  uint32_t procTx =
    micros()
    -
    startProc;

  uint32_t freeRam =
      ESP.getFreeHeap();

  // =====================================
  // LOGGING
  // =====================================
  Serial.print(
    "[TX] id="
  );

  Serial.print(
    packetId
  );

  if (
    ackReceived
  ) {

    Serial.print(
      " | RTT="
    );

    Serial.print(
      rtt
    );

    Serial.print(
      "ms | latency="
    );

    Serial.print(
      latency
    );
  }
  else {

    Serial.print(
      " | ACK_TIMEOUT"
    );
  }

  Serial.print(
    " | proc_tx="
  );

  Serial.print(
    procTx
  );

  Serial.print(
    "us | ram="
  );

  Serial.println(
    freeRam
  );

  delay(5000);
}