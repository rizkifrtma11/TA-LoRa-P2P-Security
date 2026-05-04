/*
RECEIVER
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code is for receiving data using LoRa on an ESP32. It initializes the LoRa module, sets the necessary parameters, and continuously listens for incoming packets. When a packet is received, it prints the contents to the Serial Monitor.
Note: Make sure to connect the LoRa module to the correct pins (SS, RST, DIO0) as defined in the code.
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>
#include "mbedtls/aes.h"

#define SS   5
#define RST  14
#define DIO0 26

// ===== PARAM =====
const uint32_t NUM_PACKETS = 200; 

// ===== KUNCI KRIPTOGRAFI =====
const char* MASTER_KEY = "SkripsiKi2026!!!"; // Harus persis 16 karakter
const char* IV_BASE    = "IV-LORA-2026";     // Harus persis 12 karakter

// ===== METRIK =====
uint32_t receivedCount = 0;
uint32_t uniqueReceived = 0;
uint32_t duplicateCount = 0;
uint32_t invalidCount = 0;
uint32_t rejectedCount = 0;
uint32_t totalBytes = 0;

unsigned long firstRecvTime = 0;
unsigned long lastRecvTime  = 0;
unsigned long totalLatency = 0;
unsigned long minLatency = 4294967295; 
unsigned long maxLatency = 0;

// High-Res Metrik
unsigned long totalProcTimeUs = 0;
unsigned long minProcUs = 4294967295;
unsigned long maxProcUs = 0;

bool started = false;
bool done = false;
bool receivedFlags[NUM_PACKETS] = {false};

// ===== FUNGSI KEBAL BUG BUAT HEX =====
uint8_t parseChar(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

void hexToBytes(String hex, unsigned char* byteArr) {
  for (size_t i = 0; i < hex.length() / 2; i++) {
    byteArr[i] = (parseChar(hex[i * 2]) << 4) | parseChar(hex[i * 2 + 1]);
  }
}

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433000000)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();
  Serial.println("Receiver SECURE (AES-128 CTR) Ready");
}

void loop() {
  if (done) return;

  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String payload = "";
    while (LoRa.available()) {
      payload += (char)LoRa.read();
    }
    
    unsigned long tRecv = millis();

    if (!started) {
      firstRecvTime = tRecv;
      started = true;
    }
    lastRecvTime = tRecv;

    // ==========================================
    // ===== BACA RAM & WAKTU MULAI =====
    // ==========================================
    uint32_t freeHeapAwal = ESP.getFreeHeap();
    unsigned long tStart = micros();

    int commaIdx = payload.indexOf(',');
    if (commaIdx == -1) {
      invalidCount++;
      Serial.println("[INVALID] Koma pemisah ga ketemu!");
      return;
    }

    // 1. EKSTRAK & BERSIHKAN ID (Sanitizer)
    String rawId = payload.substring(0, commaIdx);
    String cleanId = "";
    for(int i=0; i<rawId.length(); i++) {
      if(isDigit(rawId[i])) cleanId += rawId[i];
    }
    uint32_t id = cleanId.toInt();

    // 2. EKSTRAK & BERSIHKAN HEX (Sanitizer)
    String rawHex = payload.substring(commaIdx + 1);
    String cipherHex = "";
    for(int i=0; i<rawHex.length(); i++) {
      char c = rawHex[i];
      if (isxdigit(c)) cipherHex += c; // Cuma ambil huruf A-F & 0-9
    }

    // [PRINT DEBUG] Cek isi aslinya!
    Serial.print("\n[DEBUG] Clean ID: "); Serial.print(id);
    Serial.print(" | Clean Hex: "); Serial.println(cipherHex);

    if (cipherHex.length() % 2 != 0) {
      invalidCount++;
      Serial.println("[DECRYPT FAILED] Panjang Hex ganjil (Data korup di udara)");
      return;
    }
    
    size_t cipherLen = cipherHex.length() / 2;
    unsigned char ciphertext[cipherLen];
    hexToBytes(cipherHex, ciphertext);

    // 3. Siapkan Nonce CTR persis kayak Sender
    unsigned char nc[16] = {0}; 
    memcpy(nc, IV_BASE, 12); 
    nc[12] = (id >> 24) & 0xFF;
    nc[13] = (id >> 16) & 0xFF;
    nc[14] = (id >> 8)  & 0xFF;
    nc[15] = id & 0xFF;

    // 4. Dekripsi AES-128 CTR
    unsigned char stream_block[16] = {0};
    size_t nc_off = 0;
    unsigned char plainBytes[cipherLen + 1]; 
    memset(plainBytes, 0, sizeof(plainBytes));

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*)MASTER_KEY, 128); 
    mbedtls_aes_crypt_ctr(&aes, cipherLen, &nc_off, nc, stream_block, ciphertext, plainBytes);
    mbedtls_aes_free(&aes);

    String plainText = String((char*)plainBytes);

    // 5. Cek hasil dekripsi
    int p1 = plainText.indexOf(',');
    if (p1 == -1) {
      invalidCount++;
      Serial.print("[DECRYPT FAILED] Hasil acak: ");
      Serial.println(plainText);
      return;
    }
    
    unsigned long tSend = plainText.substring(0, p1).toInt();

    // ==========================================
    // ===== WAKTU SELESAI (CPU) & BACA RAM =====
    // ==========================================
    unsigned long tEnd = micros();
    uint32_t freeHeapAkhir = ESP.getFreeHeap(); 

    unsigned long procTimeUs = (tEnd > tStart) ? (tEnd - tStart) : 0;
    if (procTimeUs < minProcUs) minProcUs = procTimeUs;
    if (procTimeUs > maxProcUs) maxProcUs = procTimeUs;
    totalProcTimeUs += procTimeUs;

    // ===== LATENCY & METRIK =====
    unsigned long latency = tRecv - tSend;
    receivedCount++;
    totalBytes += payload.length();
    totalLatency += latency;

    if (latency < minLatency) minLatency = latency;
    if (latency > maxLatency) maxLatency = latency;

    if (id < NUM_PACKETS) {
      if (!receivedFlags[id]) {
        receivedFlags[id] = true;
        uniqueReceived++;
      } else {
        duplicateCount++;
      }
    }

    // ===== LOG SUKSES =====
    Serial.print("ID="); Serial.print(id);
    Serial.print(" | DEC_DATA="); Serial.print(plainText);
    Serial.print(" | LAT="); Serial.print(latency);
    Serial.print(" ms | PROC_RX="); Serial.print(procTimeUs);
    Serial.print(" us | RAM="); Serial.print(freeHeapAkhir);
    Serial.println(" Bytes");
  }

  // ===== TIMEOUT =====
  if (started && !done && (millis() - lastRecvTime > 50000)) {
    Serial.println("\n[TIMEOUT] 50 detik tidak ada paket -> sesi selesai");
    done = true;
  }
}