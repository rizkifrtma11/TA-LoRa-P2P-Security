/*
SENDER
--------------------------------
Author: Mohammad Rizki  Fadillah
Description:
This code is for sending data using LoRa communication protocol with an ESP32 microcontroller.
It initializes the LoRa module, sets the necessary parameters, and sends a message every second
--------------------------------
*/

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include "mbedtls/aes.h" // Library bawaan ESP32 buat hardware AES

#define SS   5
#define RST  14
#define DIO0 26

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// ===== PARAM UJI =====
const uint32_t NUM_PACKETS = 100;
const uint32_t INTERVAL_MS = 5000;

// ===== KUNCI KRIPTOGRAFI =====
const char* MASTER_KEY = "SkripsiKi2026!!!"; // 16 Byte AES-128 Key
const char* IV_BASE    = "IV-LORA-2026";     // 12 Byte Base IV (Sisa 4 Byte buat Counter)

// ===== STATE =====
uint32_t counter = 0;
unsigned long lastSend = 0;
bool testDone = false;

// ===== METRIK =====
uint32_t totalPayloadBytes = 0;
unsigned long totalProcTime = 0;
unsigned long minProc = 4294967295;
unsigned long maxProc = 0;

// Fungsi mengubah byte hasil enkripsi ke Hex String
String bytesToHex(unsigned char* data, size_t length) {
  String hexStr = "";
  for (size_t i = 0; i < length; i++) {
    if (data[i] < 0x10) hexStr += "0";
    hexStr += String(data[i], HEX);
  }
  hexStr.toUpperCase();
  return hexStr;
}

void setup() {
  Serial.begin(115200);

  Serial.println("Booting... Tunggu 2 detik");
  delay(2000); 

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433000000)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);

  dht.begin();

  Serial.println("Sender SECURE (AES-128 CTR) Ready. Mulai ngirim...");
}

void loop() {
  if (testDone) return;

  if (millis() - lastSend >= INTERVAL_MS) {
    lastSend = millis();

    float temp = dht.readTemperature();
    if (isnan(temp)) {
      Serial.println("DHT error, pakai dummy value");
      temp = -99.0;
    }

    const char* status = (temp > 30.0) ? "PANAS" : "AMAN";
    unsigned long tSend = millis();

    // ==========================================
    // ===== WAKTU MULAI (CPU) =====
    // ==========================================
    unsigned long tStart = micros();

    // 1. Susun Plaintext yang mau dienkripsi (TANPA COUNTER)
    char plainData[64];
    snprintf(plainData, sizeof(plainData), "%lu,%.2f,%s", tSend, temp, status);
    size_t plainLen = strlen(plainData);

    // 2. Siapkan Nonce/Counter Block untuk CTR (16 Byte)
    unsigned char nc[16] = {0}; 
    memcpy(nc, IV_BASE, 12); // Masukkan 12 byte pertama dari IV_BASE
    
    // Masukkan nilai counter ke 4 byte terakhir nc (biar IV berubah tiap paket)
    nc[12] = (counter >> 24) & 0xFF;
    nc[13] = (counter >> 16) & 0xFF;
    nc[14] = (counter >> 8)  & 0xFF;
    nc[15] = counter & 0xFF;

    // 3. Proses Enkripsi AES-128 CTR
    unsigned char stream_block[16] = {0};
    size_t nc_off = 0;
    unsigned char ciphertext[64]; // Alokasi buffer output

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const unsigned char*)MASTER_KEY, 128); // Tetap pakai setkey_enc untuk CTR
    mbedtls_aes_crypt_ctr(&aes, plainLen, &nc_off, nc, stream_block, (const unsigned char*)plainData, ciphertext);
    mbedtls_aes_free(&aes);

    // 4. Ubah ke Hex dan gabungkan dengan Counter plaintext
    String cipherHex = bytesToHex(ciphertext, plainLen);
    String finalPayload = String(counter) + "," + cipherHex;

    // ==========================================
    // ===== WAKTU SELESAI (CPU) & BACA RAM =====
    // ==========================================
    unsigned long tEnd = micros();
    uint32_t freeHeap = ESP.getFreeHeap(); 

    unsigned long procTime = (tEnd > tStart) ? (tEnd - tStart) : 0; 
    if (procTime < minProc) minProc = procTime;
    if (procTime > maxProc) maxProc = procTime;
    totalProcTime += procTime;
    
    uint32_t payloadLen = finalPayload.length();
    totalPayloadBytes += payloadLen;

    // ===== SEND =====
    LoRa.beginPacket();
    LoRa.print(finalPayload);
    LoRa.endPacket();

    // ===== LOG =====
    Serial.print("PLAIN: "); Serial.print(plainData);
    Serial.print(" | SEND: "); Serial.print(finalPayload);
    Serial.print(" | SIZE="); Serial.print(payloadLen);
    Serial.print(" | PROC_TX="); Serial.print(procTime);
    Serial.print(" us | FREE_HEAP="); Serial.print(freeHeap);
    Serial.println(" Bytes");

    counter++;

    // ===== DONE =====
    if (counter >= NUM_PACKETS) {
      Serial.println("\n=== SENDER SECURE RESULT ===");
      float avgProc = totalProcTime / (float)NUM_PACKETS;
      Serial.print("Total Packet: "); Serial.println(NUM_PACKETS);
      Serial.print("Total Payload Bytes: "); Serial.println(totalPayloadBytes);
      Serial.print("Min Proc (us): "); Serial.println(minProc);
      Serial.print("Max Proc (us): "); Serial.println(maxProc);
      Serial.print("Avg Proc (us): "); Serial.println(avgProc);
      Serial.print("Final Free Heap (Bytes): "); Serial.println(freeHeap);
      testDone = true;
    }
  }
}