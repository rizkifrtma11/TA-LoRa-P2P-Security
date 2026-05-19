/*
SENDER
--------------------------------
Author: Mohammad Rizki  Fadillah
Deskripsi:
- Menggunakan sensor DHT22 untuk membaca suhu dan kelembaban udara.
- Mengirim data suhu, kelembaban, dan status suhu (PANAS/NORMAL/DINGIN) melalui LoRa.
- Format payload: ID,TIMESTAMP,TEMP,HUM,STATUS
- ID: Nomor urut paket yang dikirim.
- TIMESTAMP: Waktu pengiriman dalam milidetik sejak program dimulai.
- TEMP: Nilai suhu dengan 2 desimal.
- HUM: Nilai kelembaban dengan 2 desimal.
- STATUS: Kategori suhu berdasarkan nilai suhu (PANAS/NORMAL/DINGIN).
- Pengiriman dilakukan setiap 5 detik.

Catatan:
- Pastikan untuk menghubungkan sensor DHT22 ke pin yang sesuai (DHTPIN) dan LoRa ke pin SS, RST, DIO0 yang telah ditentukan.
- Sesuaikan frekuensi LoRa (433E6) jika menggunakan frekuensi yang berbeda.
- Pastikan library LoRa dan DHT sensor library sudah terinstal di lingkungan pengembangan Anda.
--------------------------------
*/
/*
RECEIVER
--------------------------------
Author: Mohammad Rizki Fadillah

SECURITY FEATURES:
- AES-128 CTR Decryption
- HMAC-SHA256 Verification
- Counter-Based Nonce
- Dynamic Derived Key Function
- Replay Attack Detection

FINAL PAYLOAD FORMAT:
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
// LAST COUNTER
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
// HEX TO BYTES
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
// BYTES TO HEX
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

  LoRa.setPins(
    SS,
    RST,
    DIO0
  );

  if (!LoRa.begin(433E6)) {

    Serial.println(
      "LoRa init failed!"
    );

    while (1);
  }

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(125E3);

  Serial.println(
    "================================"
  );

  Serial.println(
    "SECURE RECEIVER READY"
  );

  Serial.println(
    "AES + HMAC + ANTI REPLAY"
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

  if (packetSize) {

    String payload = "";

    while (LoRa.available()) {

      payload +=
        (char)LoRa.read();
    }

    // =====================================
    // SPLIT:
    // COUNTER,CIPHER,HMAC
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
        "[ERROR] Invalid format"
      );

      return;
    }

    String counterStr =
      payload.substring(0, p1);

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
      counterStr + "," +
      cipherHex;

    String calcHmac =
      hmacSHA256(authData);

    if (
      calcHmac != recvHmac
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
      counter <= lastCounter
    ) {

      Serial.print(
        "[DROP] REPLAY DETECTED | COUNTER="
      );

      Serial.println(counter);

      LoRa.receive();

      return;
    }

    lastCounter = counter;

    // =====================================
    // HEX CLEAN
    // =====================================
    String cleanHex = "";

    for (
      int i = 0;
      i < cipherHex.length();
      i++
    ) {

      if (
        isxdigit(cipherHex[i])
      ) {

        cleanHex +=
          cipherHex[i];
      }
    }

    if (
      cleanHex.length() % 2 != 0
    ) {

      Serial.println(
        "[ERROR] HEX CORRUPTED"
      );

      return;
    }

    // =====================================
    // HEX -> BYTES
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
    // NONCE
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

    // =====================================
    // AES CTR DECRYPT
    // =====================================
    unsigned char stream_block[16] = {0};

    size_t nc_off = 0;

    unsigned char plainBytes[cipherLen + 1];

    memset(
      plainBytes,
      0,
      sizeof(plainBytes)
    );

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
      String((char*)plainBytes);

    // =====================================
    // PARSE DATA
    // =====================================
    int idx1 =
      decrypted.indexOf(',');

    int idx2 =
      decrypted.indexOf(
        ',',
        idx1 + 1
      );

    long senderTimestamp =
      decrypted.substring(
        idx1 + 1,
        idx2
      ).toInt();

    // =====================================
    // METRICS
    // =====================================
    long latency =
      millis() -
      senderTimestamp;

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
    // OUTPUT
    // =====================================
    Serial.print("PAYLOAD=");
    Serial.print(decrypted);

    Serial.print("|CTR=");
    Serial.print(counter);

    Serial.print("|ENC=");
    Serial.print(cleanHex);

    Serial.print("|HMAC=");
    Serial.print(recvHmac);

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