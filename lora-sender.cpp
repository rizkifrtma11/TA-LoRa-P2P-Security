/*
SENDER
--------------------------------
Author: Mohammad Rizki Fadillah

SECURITY FEATURES:
- AES-128 CTR Encryption
- HMAC-SHA256 Integrity
- Counter-Based Nonce
- Dynamic Derived Key Function
- Replay Protection Support

FINAL PAYLOAD FORMAT:
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

long packetId = 0;

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
// COUNTER NONCE
// =====================================
uint32_t counter = 0;

// =====================================
// HEX CONVERTER
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
// DERIVED KEY FUNCTION
// SHA256(master_key + counter)
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

  // ambil 16 byte pertama
  memcpy(aesKey, shaResult, 16);
}

// =====================================
// HMAC SHA256
// =====================================
String hmacSHA256(String data) {

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

  return bytesToHex(hmacResult, 32);
}

// =====================================
// SETUP
// =====================================
void setup() {

  Serial.begin(115200);

  dht.begin();

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa init failed!");

    while (1);
  }

  LoRa.setTxPower(17);

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(125E3);

  Serial.println("================================");

  Serial.println("SECURE LORA SENDER READY");

  Serial.println("AES-128 CTR + HMAC-SHA256");

  Serial.println("Dynamic Derived Key Enabled");

  Serial.println("================================");

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

    Serial.println("Failed to read DHT");

    delay(2000);

    return;
  }

  // =====================================
  // PACKET ID
  // =====================================
  packetId++;

  unsigned long senderTimestamp =
    millis();

  // =====================================
  // STATUS
  // =====================================
  String statusData;

  if (temperature >= 33.0)
    statusData = "PANAS";

  else if (temperature >= 28.0)
    statusData = "NORMAL";

  else
    statusData = "DINGIN";

  // =====================================
  // PLAIN PAYLOAD
  // =====================================
  String payload =
      String(packetId) + "," +
      String(senderTimestamp) + "," +
      String(temperature, 2) + "," +
      String(humidity, 2) + "," +
      statusData;

  unsigned long tStart =
    micros();

  // =====================================
  // DERIVE DYNAMIC AES KEY
  // =====================================
  unsigned char dynamicKey[16];

  deriveKey(counter, dynamicKey);

  // =====================================
  // AES CTR NONCE
  // =====================================
  unsigned char nc[16] = {0};

  memcpy(nc, IV_BASE, 12);

  nc[12] = (counter >> 24) & 0xFF;
  nc[13] = (counter >> 16) & 0xFF;
  nc[14] = (counter >> 8) & 0xFF;
  nc[15] = counter & 0xFF;

  // =====================================
  // PREPARE BUFFER
  // =====================================
  char plain[128];

  strncpy(
    plain,
    payload.c_str(),
    sizeof(plain)
  );

  unsigned char stream_block[16] = {0};

  size_t nc_off = 0;

  unsigned char ciphertext[128];

  // =====================================
  // AES CTR ENCRYPTION
  // =====================================
  mbedtls_aes_context aes;

  mbedtls_aes_init(&aes);

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

  mbedtls_aes_free(&aes);

  // =====================================
  // HEX CIPHERTEXT
  // =====================================
  String cipherHex =
    bytesToHex(
      ciphertext,
      payload.length()
    );

  // =====================================
  // AUTH DATA
  // counter + ciphertext
  // =====================================
  String authData =
      String(counter) + "," +
      cipherHex;

  // =====================================
  // HMAC SHA256
  // =====================================
  String hmacHex =
    hmacSHA256(authData);

  // =====================================
  // FINAL PAYLOAD
  // COUNTER,CIPHER,HMAC
  // =====================================
  String finalPayload =
      authData + "," +
      hmacHex;

  unsigned long procTime =
    micros() - tStart;

  // =====================================
  // SEND PACKET
  // =====================================
  LoRa.beginPacket();

  LoRa.print(finalPayload);

  LoRa.endPacket();

  // =====================================
  // LOG
  // =====================================
  Serial.println("\n========== SEND ==========");

  Serial.print("PLAIN     : ");
  Serial.println(payload);

  Serial.print("COUNTER   : ");
  Serial.println(counter);

  Serial.print("CIPHER    : ");
  Serial.println(cipherHex);

  Serial.print("HMAC      : ");
  Serial.println(hmacHex);

  Serial.print("PROC TIME : ");
  Serial.print(procTime);
  Serial.println(" us");

  Serial.println("==========================");

  // =====================================
  // NEXT COUNTER
  // =====================================
  counter++;

  delay(5000);
}