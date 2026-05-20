/*
SECURE RECEIVER
--------------------------------
Author: Mohammad Rizki Fadillah

FEATURES:
- AES-128 CTR Decryption
- HMAC-SHA256 Verification
- Dynamic Key Derivation
- Anti Replay Protection
- ACK Response
- DB Compatible Output

FINAL PAYLOAD:
COUNTER,CIPHERTEXT,HMAC
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>

#include "mbedtls/aes.h"
#include "mbedtls/md.h"

#define SS    5
#define RST   14
#define DIO0  2

// =====================================
// SECURITY CONFIG
// =====================================
const char* MASTER_KEY =
  "SkripsiKi2026!!!";

const char* HMAC_KEY =
  "HMACSecure2026";

const char* IV_BASE =
  "IV-LORA-2026";

// =====================================
// ANTI REPLAY
// =====================================
uint32_t lastCounter = 0;

// =====================================
// HEX CHAR -> BYTE
// =====================================
uint8_t hexCharToByte(char c) {

  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  return 0;
}

// =====================================
// HEX -> BYTES
// =====================================
void hexToBytes(
  String hex,
  unsigned char* out
) {

  for (size_t i = 0; i < hex.length() / 2; i++) {

    out[i] =
      (hexCharToByte(hex[i * 2]) << 4)
      |
      hexCharToByte(hex[i * 2 + 1]);
  }
}

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
// DYNAMIC KEY
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

  memcpy(
    aesKey,
    shaResult,
    16
  );
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
    (const unsigned char*)
    HMAC_KEY,
    strlen(HMAC_KEY)
  );

  mbedtls_md_hmac_update(
    &ctx,
    (const unsigned char*)
    data.c_str(),
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

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(
    125E3
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "SECURE RECEIVER READY"
  );

  Serial.println(
    "AES + HMAC + ANTI REPLAY + ACK"
  );

  Serial.println(
    "================================"
  );

  LoRa.receive();
}

// =====================================
// MAIN LOOP
// =====================================
void loop() {

  uint32_t startProc =
    micros();

  int packetSize =
    LoRa.parsePacket();

  if (!packetSize)
    return;

  String payload = "";

  while (
    LoRa.available()
  ) {

    payload +=
      (char)
      LoRa.read();
  }

  // =====================================
  // SPLIT
  // =====================================
  int p1 =
    payload.indexOf(',');

  int p2 =
    payload.indexOf(
      ',',
      p1 + 1
    );

  if (
    p1 == -1 ||
    p2 == -1
  ) {

    Serial.println(
      "[ERROR] Invalid packet format"
    );

    LoRa.receive();
    return;
  }

  String counterStr =
    payload.substring(
      0,
      p1
    );

  String cipherHex =
    payload.substring(
      p1 + 1,
      p2
    );

  String recvHmac =
    payload.substring(
      p2 + 1
    );

  uint32_t counter =
    counterStr.toInt();

  // =====================================
  // HMAC VERIFY
  // =====================================
  String authData =
      counterStr +
      "," +
      cipherHex;

  String calcHmac =
    hmacSHA256(
      authData
    );

  if (
    calcHmac !=
    recvHmac
  ) {

    Serial.println(
      "[DROP] HMAC INVALID"
    );

    LoRa.receive();
    return;
  }

  // =====================================
  // ANTI REPLAY
  // =====================================
  if (
    counter <=
    lastCounter
  ) {

    Serial.print(
      "[DROP] REPLAY DETECTED | COUNTER="
    );

    Serial.println(
      counter
    );

    LoRa.receive();
    return;
  }

  lastCounter =
    counter;

  // =====================================
  // HEX CLEAN
  // =====================================
  String cleanHex =
    "";

  for (
    int i = 0;
    i < cipherHex.length();
    i++
  ) {

    if (
      isxdigit(
        cipherHex[i]
      )
    ) {

      cleanHex +=
        cipherHex[i];
    }
  }

  if (
    cleanHex.length()
    % 2 != 0
  ) {

    Serial.println(
      "[ERROR] HEX CORRUPTED"
    );

    LoRa.receive();
    return;
  }

  // =====================================
  // HEX -> BYTE
  // =====================================
  size_t cipherLen =
    cleanHex.length() / 2;

  unsigned char ciphertext[cipherLen];

  hexToBytes(
    cleanHex,
    ciphertext
  );

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

  unsigned char stream_block[16] = {0};

  size_t nc_off = 0;

  unsigned char plainBytes[cipherLen + 1];

  memset(
    plainBytes,
    0,
    sizeof(plainBytes)
  );

  // =====================================
  // AES CTR DECRYPT
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
    cipherLen,
    &nc_off,
    nc,
    stream_block,
    ciphertext,
    plainBytes
  );

  mbedtls_aes_free(&aes);

  String decrypted =
    String(
      (char*)
      plainBytes
    );

  // =====================================
  // PARSE PAYLOAD
  // id,temp,hum,status
  // =====================================
  int idx1 =
    decrypted.indexOf(',');

  int idx2 =
    decrypted.indexOf(
      ',',
      idx1 + 1
    );

  int idx3 =
    decrypted.indexOf(
      ',',
      idx2 + 1
    );

  if (
    idx1 == -1 ||
    idx2 == -1 ||
    idx3 == -1
  ) {

    Serial.println(
      "[ERROR] Payload parse failed"
    );

    LoRa.receive();
    return;
  }

  long packetId =
    decrypted.substring(
      0,
      idx1
    ).toInt();

  float temperature =
    decrypted.substring(
      idx1 + 1,
      idx2
    ).toFloat();

  float humidity =
    decrypted.substring(
      idx2 + 1,
      idx3
    ).toFloat();

  String statusData =
    decrypted.substring(
      idx3 + 1
    );

  // =====================================
  // ACK SEND
  // =====================================
  String ack =
      "ACK," +
      String(counter);

  LoRa.beginPacket();
  LoRa.print(ack);
  LoRa.endPacket();

  // balik RX mode
  LoRa.receive();

  // =====================================
  // METRICS
  // =====================================
  int rssi =
    LoRa.packetRssi();

  float snr =
    LoRa.packetSnr();

  uint32_t procRx =
    micros() -
    startProc;

  uint32_t freeRam =
    ESP.getFreeHeap();

  // =====================================
  // DB OUTPUT
  // =====================================
  Serial.print("PAYLOAD=");
  Serial.print(packetId);

  Serial.print(",");
  Serial.print(temperature, 2);

  Serial.print(",");
  Serial.print(humidity, 2);

  Serial.print(",");
  Serial.print(statusData);

  Serial.print(" | CTR=");
  Serial.print(counter);

  Serial.print(" | ENC=");
  Serial.print(cipherHex);

  Serial.print(" |HMAC=");
  Serial.print(recvHmac);

  Serial.print(" | RSSI=");
  Serial.print(rssi);

  Serial.print(" | SNR=");
  Serial.print(snr);

  Serial.print(" | PROC=");
  Serial.print(procRx);

  Serial.print(" | SIZE=");
  Serial.print(packetSize);

  Serial.print(" | RAM=");
  Serial.println(freeRam);
}