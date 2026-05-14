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

#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include "mbedtls/aes.h"

#define DHTPIN   4
#define DHTTYPE  DHT22

DHT dht(DHTPIN, DHTTYPE);

#define SS    5
#define RST   14
#define DIO0  2

long packetId = 0;

// =========================
// AES CONFIG
// =========================
const char* MASTER_KEY = "SkripsiKi2026!!!"; // 16 byte
const char* IV_BASE    = "IV-LORA-2026";     // 12 byte base
uint32_t counter = 0;

// hex converter
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

  dht.begin();

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  LoRa.setTxPower(17);
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);

  Serial.println("LoRa Sender Ready");

  delay(3000);
}

void loop() {

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read DHT");
    delay(2000);
    return;
  }

  packetId++;
  unsigned long senderTimestamp = millis();

  String statusData;

  if (temperature >= 33.0) statusData = "PANAS";
  else if (temperature >= 28.0) statusData = "NORMAL";
  else statusData = "DINGIN";

  // =========================
  // PLAIN PAYLOAD (struktur lama)
  // =========================
  String payload =
      String(packetId) + "," +
      String(senderTimestamp) + "," +
      String(temperature, 2) + "," +
      String(humidity, 2) + "," +
      statusData;

  unsigned long tStart = micros();

  // =========================
  // AES-CTR ENCRYPT
  // =========================
  char plain[128];
  strncpy(plain, payload.c_str(), sizeof(plain));

  unsigned char nc[16] = {0};
  memcpy(nc, IV_BASE, 12);

  nc[12] = (counter >> 24) & 0xFF;
  nc[13] = (counter >> 16) & 0xFF;
  nc[14] = (counter >> 8) & 0xFF;
  nc[15] = counter & 0xFF;

  unsigned char stream_block[16] = {0};
  size_t nc_off = 0;
  unsigned char ciphertext[128];

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, (const unsigned char*)MASTER_KEY, 128);

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

  String cipherHex = bytesToHex(ciphertext, payload.length());

  unsigned long procTime = micros() - tStart;

  // =========================
  // FINAL PAYLOAD (counter + encrypted data)
  // =========================
  String finalPayload = String(counter) + "," + cipherHex;

  // =========================
  // SEND
  // =========================
  LoRa.beginPacket();
  LoRa.print(finalPayload);
  LoRa.endPacket();

  // =========================
  // LOG
  // =========================
  Serial.print("SEND ENC: ");
  Serial.print(finalPayload);
  Serial.print(" | PROC=");
  Serial.print(procTime);
  Serial.println(" us");

  counter++;

  delay(5000);
}