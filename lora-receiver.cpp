/*
RECEIVER AES-128 CTR + ACK
--------------------------------
Author: Mohammad Rizki Fadillah

Description:
Receiver LoRa
+ AES-128 CTR decrypt
+ ACK response
+ vulnerable replay
+ vulnerable tampering
*/

#include <SPI.h>
#include <LoRa.h>
#include "mbedtls/aes.h"

#define SS    5
#define RST   14
#define DIO0  26

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
// sengaja vulnerable
const uint8_t AES_IV[16] = {
  0xAA,0xBB,0xCC,0xDD,
  0xEE,0xFF,0x11,0x22,
  0x33,0x44,0x55,0x66,
  0x77,0x88,0x99,0x00
};

// =====================================
// HEX -> BYTE
// =====================================
int hexToBytes(
  String hex,
  uint8_t* output
) {

  int len =
    hex.length();

  int finalLen =
    len / 2;

  for (
    int i = 0;
    i < finalLen;
    i++
  ) {

    String byteStr =
      hex.substring(
        i * 2,
        i * 2 + 2
      );

    output[i] =
      strtol(
        byteStr.c_str(),
        NULL,
        16
      );
  }

  return finalLen;
}

// =====================================
// AES CTR DECRYPT
// =====================================
String decryptAESCTR(
  String ciphertextHex
) {

  mbedtls_aes_context aes;

  mbedtls_aes_init(
    &aes
  );

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

  size_t nc_off =
    0;

  int cipherLen =
    ciphertextHex.length() / 2;

  uint8_t cipherBytes[cipherLen];
  uint8_t output[cipherLen];

  hexToBytes(
    ciphertextHex,
    cipherBytes
  );

  mbedtls_aes_crypt_ctr(
    &aes,
    cipherLen,
    &nc_off,
    nonce_counter,
    stream_block,
    cipherBytes,
    output
  );

  mbedtls_aes_free(
    &aes
  );

  String plaintext =
    "";

  for (
    int i = 0;
    i < cipherLen;
    i++
  ) {

    plaintext +=
      (char)
      output[i];
  }

  return plaintext;
}

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(
    115200
  );

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

  Serial.println(
    "================================"
  );

  Serial.println(
    "RECEIVER AES CTR READY"
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

  // =====================================
  // READ CIPHERTEXT
  // =====================================
  String ciphertext =
    "";

  while (
    LoRa.available()
  ) {

    ciphertext +=
      (char)
      LoRa.read();
  }

  // =====================================
  // AES DECRYPT
  // =====================================
  String plaintext =
      decryptAESCTR(
        ciphertext
      );

  // format:
  // id,temp,hum,status
  int p1 =
    plaintext.indexOf(',');

  if (
    p1 == -1
  ) {

    Serial.println(
      "[ERROR] Invalid decrypt"
    );

    LoRa.receive();
    return;
  }

  uint32_t packetId =
    plaintext.substring(
      0,
      p1
    ).toInt();

  // =====================================
  // SEND ACK
  // =====================================
  String ack =
      "ACK,"
      +
      String(
        packetId
      );

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

  Serial.print(
    " | packet_size="
  );

  Serial.print(
    packetSize
  );

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
    plaintext
  );
}