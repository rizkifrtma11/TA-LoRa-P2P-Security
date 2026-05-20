/*
SECURE LORA SENDER
--------------------------------
Author: Mohammad Rizki Fadillah

FEATURES:
- AES-128 CTR Encryption
- HMAC-SHA256 Integrity
- Dynamic Key Derivation
- Counter Nonce
- ACK Based Latency (RTT/2)

FINAL PAYLOAD:
COUNTER,CIPHERTEXT,HMAC
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

#include "mbedtls/aes.h"
#include "mbedtls/md.h"

#define DHTPIN   4
#define DHTTYPE  DHT22

DHT dht(DHTPIN, DHTTYPE);

#define SS    5
#define RST   14
#define DIO0  2

// =====================================
// CONFIG
// =====================================
const unsigned long ACK_TIMEOUT = 3000;

long packetId = 0;
uint32_t counter = 0;

// =====================================
// MASTER KEY
// =====================================
const char* MASTER_KEY =
  "SkripsiKi2026!!!";

// =====================================
// HMAC KEY
// =====================================
const char* HMAC_KEY =
  "HMACSecure2026";

// =====================================
// IV BASE
// =====================================
const char* IV_BASE =
  "IV-LORA-2026";

// =====================================
// BYTES -> HEX
// =====================================
String bytesToHex(
  unsigned char* data,
  size_t length
) {

  String hexStr = "";

  for (size_t i = 0; i < length; i++) {

    if (data[i] < 0x10)
      hexStr += "0";

    hexStr += String(data[i], HEX);
  }

  hexStr.toUpperCase();

  return hexStr;
}

// =====================================
// DYNAMIC AES KEY
// SHA256(master + counter)
// ambil 16 byte pertama
// =====================================
void deriveKey(
  uint32_t counter,
  unsigned char* aesKey
) {

  String seed =
      String(MASTER_KEY) +
      String(counter);

  unsigned char shaResult[32];

  mbedtls_md(
    mbedtls_md_info_from_type(
      MBEDTLS_MD_SHA256
    ),
    (const unsigned char*)seed.c_str(),
    seed.length(),
    shaResult
  );

  memcpy(aesKey, shaResult, 16);
}

// =====================================
// HMAC SHA256
// =====================================
String hmacSHA256(
  String data
) {

  unsigned char hmacResult[32];

  mbedtls_md_context_t ctx;

  mbedtls_md_init(&ctx);

  mbedtls_md_setup(
    &ctx,
    mbedtls_md_info_from_type(
      MBEDTLS_MD_SHA256
    ),
    1
  );

  mbedtls_md_hmac_starts(
    &ctx,
    (const unsigned char*)HMAC_KEY,
    strlen(HMAC_KEY)
  );

  mbedtls_md_hmac_update(
    &ctx,
    (const unsigned char*)data.c_str(),
    data.length()
  );

  mbedtls_md_hmac_finish(
    &ctx,
    hmacResult
  );

  mbedtls_md_free(&ctx);

  return bytesToHex(
    hmacResult,
    32
  );
}

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);

  dht.begin();

  LoRa.setPins(
    SS,
    RST,
    DIO0
  );

  if (
    !LoRa.begin(433E6)
  ) {

    Serial.println(
      "LoRa init failed!"
    );

    while (1);
  }

  LoRa.setTxPower(17);

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(
    125E3
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "SECURE LORA SENDER READY"
  );

  Serial.println(
    "AES-128 CTR + HMAC"
  );

  Serial.println(
    "ACK RTT LATENCY ENABLED"
  );

  Serial.println(
    "================================"
  );

  delay(3000);
}

// =====================================
// MAIN LOOP
// =====================================
void loop() {

  // =====================================
  // READ SENSOR
  // =====================================
  float temperature =
    dht.readTemperature();

  float humidity =
    dht.readHumidity();

  if (
    isnan(temperature) ||
    isnan(humidity)
  ) {

    Serial.println(
      "Failed to read DHT"
    );

    delay(2000);

    return;
  }

  packetId++;

  // =====================================
  // STATUS
  // =====================================
  String statusData;

  if (temperature >= 33.0)
    statusData = "PANAS";

  else if (
    temperature >= 28.0
  )
    statusData = "NORMAL";

  else
    statusData = "DINGIN";

  // =====================================
  // PLAINTEXT PAYLOAD
  // id,temp,hum,status
  // =====================================
  String payload =
      String(packetId) + "," +
      String(temperature, 2) + "," +
      String(humidity, 2) + "," +
      statusData;

  // =====================================
  // START PROCESS TIMER
  // =====================================
  uint32_t procStart =
    micros();

  // =====================================
  // DERIVE AES KEY
  // =====================================
  unsigned char dynamicKey[16];

  deriveKey(
    counter,
    dynamicKey
  );

  // =====================================
  // NONCE CTR
  // =====================================
  unsigned char nc[16] = {0};

  memcpy(
    nc,
    IV_BASE,
    12
  );

  nc[12] =
    (counter >> 24) & 0xFF;

  nc[13] =
    (counter >> 16) & 0xFF;

  nc[14] =
    (counter >> 8) & 0xFF;

  nc[15] =
    counter & 0xFF;

  char plain[128];

  strncpy(
    plain,
    payload.c_str(),
    sizeof(plain)
  );

  unsigned char ciphertext[128];

  unsigned char stream_block[16] = {0};

  size_t nc_off = 0;

  // =====================================
  // AES CTR ENCRYPTION
  // =====================================
  mbedtls_aes_context aes;

  mbedtls_aes_init(
    &aes
  );

  mbedtls_aes_setkey_enc(
    &aes,
    dynamicKey,
    128
  );

  mbedtls_aes_crypt_ctr(
    &aes,
    payload.length(),
    &nc_off,
    nc,
    stream_block,
    (const unsigned char*)plain,
    ciphertext
  );

  mbedtls_aes_free(
    &aes
  );

  // =====================================
  // HEX CIPHER
  // =====================================
  String cipherHex =
    bytesToHex(
      ciphertext,
      payload.length()
    );

  // =====================================
  // HMAC DATA
  // =====================================
  String authData =
      String(counter) +
      "," +
      cipherHex;

  String hmacHex =
    hmacSHA256(
      authData
    );

  // =====================================
  // FINAL PAYLOAD
  // =====================================
  String finalPayload =
      authData +
      "," +
      hmacHex;

  uint32_t procTime =
    micros() -
    procStart;

  // =====================================
  // SEND + RTT TIMER
  // =====================================
  uint32_t t0 =
    millis();

  LoRa.beginPacket();

  LoRa.print(
    finalPayload
  );

  LoRa.endPacket();

  // balik receive
  LoRa.receive();

  bool ackReceived =
    false;

  uint32_t rtt = 0;
  uint32_t latency = 0;

  while (
    millis() - t0 <
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
          String(counter);

      if (
        ack ==
        expectedAck
      ) {

        ackReceived =
          true;

        rtt =
          millis() -
          t0;

        latency =
          rtt / 2;

        break;
      }
    }
  }

  // =====================================
  // LOG
  // =====================================
  Serial.print(
    "PAYLOAD="
  );
  Serial.print(
    payload
  );

  Serial.print(
    " | COUNTER="
  );
  Serial.print(
    counter
  );

  Serial.print(
    " | CIPHER="
  );
  Serial.print(
    cipherHex
  );

  Serial.print(
    " | HMAC="
  );
  Serial.print(
    hmacHex
  );

  Serial.print(
    " | PROC TX="
  );
  Serial.print(
    procTime
  );
  Serial.print(
    " us | "
  );

  if (
    ackReceived
  ) {

    Serial.print(
      "RTT="
    );
    Serial.print(
      rtt
    );
    Serial.print(
      " ms | "
    );

    Serial.print(
      "LATENCY="
    );
    Serial.print(
      latency
    );
    Serial.print(
      " ms"
    );

    Serial.println();
  }
  else {
    Serial.print(
      "ACK TIMEOUT"
    );
    Serial.println();
  }

  counter++;

  delay(5000);
}