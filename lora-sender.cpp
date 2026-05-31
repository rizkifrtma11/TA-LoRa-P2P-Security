/*
SENDER AES-128 CTR + ACK + DHT22
--------------------------------
Author: Mohammad Rizki Fadillah

Description:
LoRa + AES-128 CTR
+ ACK RTT Latency
+ Vulnerable Replay
+ Vulnerable Tampering
*/

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include "mbedtls/aes.h"

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
// AES CONFIG
// =====================================
const uint8_t AES_KEY[16] = {
  0x12,0x34,0x56,0x78,
  0x90,0xAB,0xCD,0xEF,
  0x11,0x22,0x33,0x44,
  0x55,0x66,0x77,0x88
};

// static IV
// sengaja vulnerable buat penelitian
const uint8_t AES_IV[16] = {
  0xAA,0xBB,0xCC,0xDD,
  0xEE,0xFF,0x11,0x22,
  0x33,0x44,0x55,0x66,
  0x77,0x88,0x99,0x00
};

// =====================================
// AES CTR ENCRYPT
// =====================================
String encryptAESCTR(String plaintext) {

  mbedtls_aes_context aes;

  mbedtls_aes_init(&aes);

  mbedtls_aes_setkey_enc(
    &aes,
    AES_KEY,
    128
  );

  uint8_t nonce_counter[16];
  memcpy(
    nonce_counter,
    AES_IV,
    16
  );

  uint8_t stream_block[16];

  size_t nc_off = 0;

  int len =
    plaintext.length();

  uint8_t input[len];
  uint8_t output[len];

  memcpy(
    input,
    plaintext.c_str(),
    len
  );

  mbedtls_aes_crypt_ctr(
    &aes,
    len,
    &nc_off,
    nonce_counter,
    stream_block,
    input,
    output
  );

  mbedtls_aes_free(
    &aes
  );

  // convert ke HEX
  String hexCipher =
    "";

  for (
    int i = 0;
    i < len;
    i++
  ) {

    char buf[3];

    sprintf(
      buf,
      "%02X",
      output[i]
    );

    hexCipher +=
      buf;
  }

  return hexCipher;
}

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

  if (
    !LoRa.begin(
      433E6
    )
  ) {

    Serial.println(
      "LoRa init FAILED!"
    );

    while (1);
  }

  LoRa.setSpreadingFactor(
    9
  );

  LoRa.setSignalBandwidth(
    125E3
  );

  LoRa.setCodingRate4(
    5
  );

  LoRa.setTxPower(
    17
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "SENDER AES CTR READY"
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
    isnan(t)
    ||
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

  // =====================================
  // PLAINTEXT
  // =====================================
  String plaintext =
      String(packetId)
      + "," +
      String(t, 1)
      + "," +
      String(h, 1)
      + "," +
      status_data;

  // =====================================
  // AES ENCRYPT
  // =====================================
  String ciphertext =
      encryptAESCTR(
        plaintext
      );

  // =====================================
  // SEND PACKET
  // =====================================
  uint32_t t0 =
    millis();

  LoRa.beginPacket();

  LoRa.print(
    ciphertext
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

  uint32_t latency =
    0;

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
          "ACK,"
          +
          String(
            packetId
          );

      if (
        ack ==
        expectedAck
      ) {

        ackReceived =
          true;

        rtt =
          millis()
          -
          t0;

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

  Serial.print(
    " | cipher="
  );

  Serial.print(
    ciphertext
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