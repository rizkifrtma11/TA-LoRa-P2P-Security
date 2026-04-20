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

#define SS   5
#define RST  14
#define DIO0 26

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

uint32_t counter = 0;

void setup() {
  Serial.begin(115200);

  SPI.begin(18, 19, 23, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init FAILED!");
    while (1);
  }

  LoRa.setTxPower(17);   // 🔥 biar sinyal kuat

  dht.begin();

  Serial.println("Sender Ready");
}

void loop() {
  float temp = dht.readTemperature();

  // 🔥 HANDLE ERROR SENSOR
  if (isnan(temp)) {
    Serial.println("DHT error!");
    return;
  }

  String status;
  if (temp > 30) {
    status = "PANAS";
  } else {
    status = "AMAN";
  }

  unsigned long timestamp = millis();

  String payload = String(counter) + "," +
                   String(timestamp) + "," +
                   String(temp, 2) + "," +   // 🔥 2 digit desimal
                   status;

  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  Serial.println("Kirim: " + payload);

  counter++;

  delay(20000);
}
