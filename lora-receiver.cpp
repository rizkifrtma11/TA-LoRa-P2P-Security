/*
RECEIVER
--------------------------------
Author: Mohammad Rizki  Fadillah
Deskripsi:
Kode ini merupakan contoh penerima (receiver) untuk komunikasi LoRa. Kode ini akan menerima paket data yang dikirim oleh perangkat lain melalui LoRa, kemudian memproses payload yang diterima, menghitung metrik seperti latensi, RSSI, SNR, waktu pemrosesan, dan sisa RAM, serta menampilkan informasi tersebut di serial monitor. Lalu menyimpan data di database untuk analisis lebih lanjut.
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>
#include "mbedtls/aes.h"

#define SS    5
#define RST   14
#define DIO0  2

// =========================
// AES CONFIG
// =========================
const char* MASTER_KEY = "SkripsiKi2026!!!";
const char* IV_BASE    = "IV-LORA-2026";

// =========================
// HEX CONVERTER
// =========================
uint8_t hexCharToByte(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

void hexToBytes(String hex, unsigned char* out) {
  for (size_t i = 0; i < hex.length() / 2; i++) {
    out[i] = (hexCharToByte(hex[i * 2]) << 4) | hexCharToByte(hex[i * 2 + 1]);
  }
}

void setup() {

  Serial.begin(115200);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa init failed!");

    while (1);
  }

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
    // SPLIT ID & HEX CIPHERTEXT
    // =========================
    int commaIdx = payload.indexOf(',');

    String idStr = payload.substring(0, commaIdx);
    String hexStr = payload.substring(commaIdx + 1);

    uint32_t id = idStr.toInt();

    // =========================
    // CLEAN HEX
    // =========================
    String cleanHex = "";
    for (int i = 0; i < hexStr.length(); i++) {
      if (isxdigit(hexStr[i])) cleanHex += hexStr[i];
    }

    if (cleanHex.length() % 2 != 0) {
      Serial.println("[ERROR] HEX corrupted");
      return;
    }

    // =========================
    // STORE ENCRYPTED HEX (FOR DB)
    // =========================
    String encrypt_bytes = cleanHex;

    // =========================
    // HEX -> BYTE
    // =========================
    size_t cipherLen = cleanHex.length() / 2;
    unsigned char ciphertext[cipherLen];
    hexToBytes(cleanHex, ciphertext);

    // =========================
    // BUILD NONCE (CTR)
    // =========================
    unsigned char nc[16] = {0};
    memcpy(nc, IV_BASE, 12);

    nc[12] = (id >> 24) & 0xFF;
    nc[13] = (id >> 16) & 0xFF;
    nc[14] = (id >> 8) & 0xFF;
    nc[15] = id & 0xFF;

    // =========================
    // AES DECRYPT CTR
    // =========================
    unsigned char stream_block[16] = {0};
    size_t nc_off = 0;

    unsigned char plainBytes[cipherLen + 1];
    memset(plainBytes, 0, sizeof(plainBytes));

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*)MASTER_KEY, 128);

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

    String decrypted = String((char*)plainBytes);

    // =========================
    // PARSE DECRYPTED DATA
    // =========================
    int idx1 = decrypted.indexOf(',');
    int idx2 = decrypted.indexOf(',', idx1 + 1);

    long senderTimestamp = decrypted.substring(idx1 + 1, idx2).toInt();

    // =========================
    // METRICS
    // =========================
    long latency = millis() - senderTimestamp;

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    uint32_t procRx = micros() - startProc;
    uint32_t freeRam = ESP.getFreeHeap();

    // =========================
    // OUTPUT (UPDATED)
    // =========================
    Serial.print("PAYLOAD=");
    Serial.print(decrypted);

    Serial.print("|ENC=");
    Serial.print(encrypt_bytes);

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