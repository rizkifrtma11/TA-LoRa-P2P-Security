/*
RECEIVER FULL SECURITY + ACK
--------------------------------
Author: Mohammad Rizki Fadillah

Description:
LoRa P2P Receiver mitigasi MITM:
1. AES-128 Mode CTR (Dekripsi)
2. HMAC-SHA256 (Validasi Integritas)
3. Counter-Based Nonce (Anti-Replay)
4. Dynamic Key Management / KDF (Stateless Sync)
*/

#include <SPI.h>
#include <LoRa.h>
#include "mbedtls/aes.h"
#include "mbedtls/md.h"

#define SS    5
#define RST   14
#define DIO0  26

// =====================================
// SECURITY KEYS & CONFIG
// =====================================
const char* MASTER_KEY = "MasterKeySkripsiRizki1234567890!"; // 32 Bytes
const char* HMAC_KEY   = "IniKunciRahasiaHMACSkripsiRizki!"; // 32 Bytes

uint8_t SESSION_KEY[16]; 
const uint8_t BASE_NONCE[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};

uint32_t lastPacketId = 0;
uint32_t currentBaseId = 0xFFFFFFFF; // Set awal ke nilai tidak valid agar memicu sinkronisasi pertama

// =====================================
// FUNGSI KDF (Dynamic Key Sync)
// =====================================
void updateSessionKey(uint32_t base_id) {
  uint8_t hmac_result[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)MASTER_KEY, 32);

  uint8_t counter_bytes[4];
  counter_bytes[0] = (base_id >> 24) & 0xFF;
  counter_bytes[1] = (base_id >> 16) & 0xFF;
  counter_bytes[2] = (base_id >> 8) & 0xFF;
  counter_bytes[3] = base_id & 0xFF;

  mbedtls_md_hmac_update(&ctx, counter_bytes, 4);
  mbedtls_md_hmac_finish(&ctx, hmac_result);
  mbedtls_md_free(&ctx);

  memcpy(SESSION_KEY, hmac_result, 16);
  currentBaseId = base_id;

  Serial.print("[-] KDF SYNCED! Session Key aktif untuk Base ID: ");
  Serial.println(base_id);
}

// =====================================
// SETUP
// =====================================
void setup() {
  Serial.begin(115200);
  while (!Serial);

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x34); // Default (Kondisi Rentan)

  Serial.println("================================");
  Serial.println("RECEIVER FULL SECURITY READY");
  Serial.println("AES-128 + HMAC + KDF DYNAMIC KEY");
  Serial.println("================================");

  LoRa.receive();
}

// =====================================
// LOOP
// =====================================
void loop() {
  int packetSize = LoRa.parsePacket();

  // Filter paket tidak masuk akal (Minimal 4 bytes Counter + 1 byte Cipher + 32 bytes HMAC)
  if (packetSize < 37) return;

  uint32_t startProc = micros();

  // 1. Ekstrak Komponen Data dari Udara
  uint8_t counter_bytes[4];
  for (int i = 0; i < 4; i++) {
    counter_bytes[i] = LoRa.read();
  }

  size_t ct_len = packetSize - 36;
  uint8_t ciphertext[ct_len];
  for (size_t i = 0; i < ct_len; i++) {
    ciphertext[i] = LoRa.read();
  }

  uint8_t hmac_received[32];
  for (int i = 0; i < 32; i++) {
    hmac_received[i] = LoRa.read();
  }

  // Rekonstruksi Packet ID
  uint32_t packetId = ((uint32_t)counter_bytes[0] << 24) |
                      ((uint32_t)counter_bytes[1] << 16) |
                      ((uint32_t)counter_bytes[2] << 8)  |
                      ((uint32_t)counter_bytes[3]);

  // 2. VALIDASI INTEGRITAS (HMAC-SHA256)
  uint8_t hmac_calc[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char*)HMAC_KEY, 32);
  mbedtls_md_hmac_update(&ctx, counter_bytes, 4);
  mbedtls_md_hmac_update(&ctx, ciphertext, ct_len);
  mbedtls_md_hmac_finish(&ctx, hmac_calc);
  mbedtls_md_free(&ctx);

  if (memcmp(hmac_calc, hmac_received, 32) != 0) {
    Serial.print("[RX] id="); Serial.print(packetId);
    Serial.println(" | DROP: HMAC INVALID (Data Tampering Detected!)");
    LoRa.receive();
    return;
  }

  // 3. PENCEGAHAN REPLAY ATTACK
  if (packetId <= lastPacketId) {
    Serial.print("[RX] id="); Serial.print(packetId);
    Serial.println(" | DROP: REPLAY DETECTED (Paket Expired atau Sudah Pernah Diterima)");
    
    // Tetap kirim ACK agar Sender berhenti melakukan retry pada paket lama
    LoRa.beginPacket();
    LoRa.print("ACK," + String(packetId));
    LoRa.endPacket();
    LoRa.receive();
    return;
  }
  
  // Lolos semua keamanan, perbarui riwayat paket terakhir
  lastPacketId = packetId;

  // 4. SINKRONISASI KDF DYNAMIC KEY
  // Rumus untuk menyamakan Base ID dengan Sender (0 untuk id 1-10, 11 untuk id 11-20, dst)
  uint32_t base_id = (packetId <= 10) ? 0 : ((packetId - 1) / 10) * 10 + 1;
  if (base_id != currentBaseId) {
    updateSessionKey(base_id);
  }

  // 5. DEKRIPSI AES-128 CTR
  uint8_t iv[16];
  memcpy(iv, BASE_NONCE, 12);
  memcpy(iv + 12, counter_bytes, 4);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, SESSION_KEY, 128); // Gunakan Session Key
  
  uint8_t plaintext[ct_len + 1]; // +1 untuk Null-Terminator String
  size_t nc_off = 0;
  uint8_t stream_block[16] = {0};
  
  mbedtls_aes_crypt_ctr(&aes, ct_len, &nc_off, iv, stream_block, ciphertext, plaintext);
  mbedtls_aes_free(&aes);
  
  plaintext[ct_len] = '\0';
  String payload = String((char*)plaintext);

  // =====================================
  // SEND ACK
  // =====================================
  String ack = "ACK," + String(packetId);
  LoRa.beginPacket();
  LoRa.print(ack);
  LoRa.endPacket();
  LoRa.receive();

  // =====================================
  // METRICS & LOGGING
  // =====================================
  uint32_t procRx = micros() - startProc;
  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();
  uint32_t freeRam = ESP.getFreeHeap();

  Serial.print("[RX] id=");
  Serial.print(packetId);
  Serial.print(" | payload=");
  Serial.print(payload);
  Serial.print(" | proc_rx=");
  Serial.print(procRx);
  Serial.print("us | packet_size=");
  Serial.print(packetSize);
  Serial.print(" | ram=");
  Serial.print(freeRam);
  Serial.print(" | rssi=");
  Serial.print(rssi);
  Serial.print(" | snr=");
  Serial.println(snr);
}