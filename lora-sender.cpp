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

#define DHTPIN   4
#define DHTTYPE  DHT22

DHT dht(DHTPIN, DHTTYPE);

#define SS    5
#define RST   14
#define DIO0  2

long packetId = 0;

void setup() {

  Serial.begin(115200);

  dht.begin();

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }

  // =========================
  // LoRa Configuration
  // =========================
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

  if (temperature >= 33.0) {
    statusData = "PANAS";
  }
  else if (temperature >= 28.0) {
    statusData = "NORMAL";
  }
  else {
    statusData = "DINGIN";
  }

  // =========================
  // Payload Format:
  // ID,TIMESTAMP,TEMP,HUM,STATUS
  // =========================
  String payload =
      String(packetId) + "," +
      String(senderTimestamp) + "," +
      String(temperature, 2) + "," +
      String(humidity, 2) + "," +
      statusData;

  // =========================
  // Send Packet
  // =========================
  LoRa.beginPacket();

  LoRa.print(payload);

  LoRa.endPacket();

  // =========================
  // Serial Monitor Log
  // =========================
  Serial.print("SEND: ");

  Serial.println(payload);

  delay(5000);
}