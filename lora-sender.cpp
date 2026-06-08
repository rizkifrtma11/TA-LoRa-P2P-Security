/*
SENDER FULL SECURITY + ACK + DHT22
--------------------------------
Author: Mohammad Rizki Fadillah

Description:
LoRa P2P dengan mitigasi MITM:
1. AES-128 Mode CTR (Kerahasiaan)
2. HMAC-SHA256 (Integritas)
3. Counter-Based Nonce (Anti-Replay)
4. Dynamic Key Management / KDF (Ganti kunci tiap 10 paket)
*/

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include "mbedtls/aes.h"
#include "mbedtls/md.h"

#define SS    5
#define RST   14
#define DIO0  26

#define DHTPIN   4
#define DHTTYPE  DHT11

DHT dht(DHTPIN, DHTTYPE);

// =====================================
// SECURITY KEYS & CONFIG
// =====================================
// Master Key & HMAC Key (Statis, ditanam di memori, tidak pernah dikirim ke udara)
const char* MASTER_KEY = "MasterKeySkripsiRizki1234567890!"; // 32 Bytes
const char* HMAC_KEY   = "IniKunciRahasiaHMACSkripsiRizki!"; // 32 Bytes

// Variabel Kunci Sesi yang akan terus berubah (Dinamis)
uint8_t SESSION_KEY[16]; // 16 Byte (128 bit) untuk AES-128

// Base Nonce / IV untuk AES-CTR (12 Bytes statis, 4 Bytes dinamis dari counter)
const uint8_t BASE_NONCE[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};

const unsigned long ACK_TIMEOUT = 3000;
uint32_t packetId = 0; // Berfungsi sebagai Sequence Counter

// =====================================
// FUNGSI KDF (Dynamic Key Management)
// =====================================
void updateSessionKey() {
  uint8_t hmac_result[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  
  // HMAC-SHA256 menggunakan Master Key
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)MASTER_KEY, 32);

  // Masukkan packetId sebagai parameter dinamis (material KDF)
  uint8_t counter_bytes[4];
  counter_bytes[0] = (packetId >> 24) & 0xFF;
  counter_bytes[1] = (packetId >> 16) & 0xFF;
  counter_bytes[2] = (packetId >> 8) & 0xFF;
  counter_bytes[3] = packetId & 0xFF;

  mbedtls_md_hmac_update(&ctx, counter_bytes, 4);
  mbedtls_md_hmac_finish(&ctx, hmac_result);
  mbedtls_md_free(&ctx);

  // Ekstrak 16 Byte pertama dari hasil Hash untuk jadi Session Key AES-128
  memcpy(SESSION_KEY, hmac_result, 16);

  Serial.print("[-] KDF TRIGGERED! Session Key baru dibuat pada Packet ID: ");
  Serial.println(packetId);
}

// =====================================
// SETUP
// =====================================
void setup() {
  Serial.begin(115200);
  while (!Serial);

  dht.begin();

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  // Konfigurasi LoRa (Spreading Factor, Bandwidth, Coding Rate, Sync Word, Tx Power)
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x34); // Default
  LoRa.setTxPower(17);

  Serial.println("================================");
  Serial.println("SENDER FULL SECURITY READY");
  Serial.println("AES-128 + HMAC + KDF DYNAMIC KEY");
  Serial.println("================================");

  // Inisialisasi Kunci Sesi Pertama (Untuk paket 1-10)
  updateSessionKey();

  delay(5000);
}

// =====================================
// LOOP
// =====================================
void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Gagal baca DHT22");
    delay(2000);
    return;
  }

  // =====================================
  // CPU PROCESS START
  // =====================================
  uint32_t startProc = micros();
  
  packetId++; // Nonce bertambah setiap 1 paket

  // TRIGGER KDF: Ganti kunci tiap kelipatan 10 (paket ke-11, 21, 31, dst)
  if (packetId > 1 && (packetId - 1) % 10 == 0) {
    updateSessionKey();
  }

  String status_data = (t > 30.0) ? "PANAS" : "NORMAL";

  // 1. Siapkan Plaintext
  String pt_str = String(packetId) + "," + String(t, 1) + "," + String(h, 1) + "," + status_data;
  size_t pt_len = pt_str.length();
  uint8_t plaintext[pt_len];
  pt_str.getBytes(plaintext, pt_len + 1);

  // 2. Siapkan IV / Nonce (12 Byte Base + 4 Byte Packet ID)
  uint8_t iv[16];
  memcpy(iv, BASE_NONCE, 12);
  iv[12] = (packetId >> 24) & 0xFF;
  iv[13] = (packetId >> 16) & 0xFF;
  iv[14] = (packetId >> 8) & 0xFF;
  iv[15] = packetId & 0xFF;

  // Array counter khusus untuk disematkan di udara (4 Byte)
  uint8_t counter_bytes[4] = {iv[12], iv[13], iv[14], iv[15]};

  // 3. Proses Enkripsi AES-128 CTR (Menggunakan SESSION_KEY Dinamis)
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, SESSION_KEY, 128); // <-- Memakai kunci dinamis
  
  uint8_t ciphertext[pt_len];
  size_t nc_off = 0;
  uint8_t stream_block[16] = {0};
  
  mbedtls_aes_crypt_ctr(&aes, pt_len, &nc_off, iv, stream_block, plaintext, ciphertext);
  mbedtls_aes_free(&aes);

  // 4. Proses Autentikasi HMAC-SHA256
  uint8_t hmac_result[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)HMAC_KEY, 32);
  
  // HMAC mengkalkulasi Counter + Ciphertext (Anti-Tampering)
  mbedtls_md_hmac_update(&ctx, counter_bytes, 4);
  mbedtls_md_hmac_update(&ctx, ciphertext, pt_len);
  mbedtls_md_hmac_finish(&ctx, hmac_result);
  mbedtls_md_free(&ctx);

  // Log Ciphertext
  String cipherHex = "";
  for(size_t i = 0; i < pt_len; i++) {
    if(ciphertext[i] < 16) cipherHex += "0";
    cipherHex += String(ciphertext[i], HEX);
  }
  cipherHex.toUpperCase();

  // =====================================
  // SEND PACKET
  // =====================================
  uint32_t t0 = millis();

  LoRa.beginPacket();
  // Format Payload di Udara: [Counter 4B] + [Ciphertext] + [HMAC 32B]
  LoRa.write(counter_bytes, 4);
  LoRa.write(ciphertext, pt_len);
  LoRa.write(hmac_result, 32);
  LoRa.endPacket();

  LoRa.receive();

  // =====================================
  // WAIT ACK
  // =====================================
  bool ackReceived = false;
  uint32_t rtt = 0;
  uint32_t latency = 0;

  while (millis() - t0 < ACK_TIMEOUT) {
    int packetSize = LoRa.parsePacket();
    
    if (packetSize) {
      String ack = "";
      while (LoRa.available()) {
        ack += (char)LoRa.read();
      }

      String expectedAck = "ACK," + String(packetId);

      if (ack == expectedAck) {
        ackReceived = true;
        rtt = millis() - t0;
        latency = rtt / 2;
        break;
      }
    }
  }

  // =====================================
  // METRICS & LOGGING
  // =====================================
  uint32_t procTx = micros() - startProc;
  uint32_t freeRam = ESP.getFreeHeap();

  Serial.print("[TX] id=");
  Serial.print(packetId);
  Serial.print(" | cipher=");
  Serial.print(cipherHex);

  if (ackReceived) {
    Serial.print(" | RTT=");
    Serial.print(rtt);
    Serial.print("ms | latency=");
    Serial.print(latency);
  } else {
    Serial.print(" | ACK_TIMEOUT");
  }

  Serial.print(" | proc_tx=");
  Serial.print(procTx);
  Serial.print("us | ram=");
  Serial.println(freeRam);

  delay(5000);
}