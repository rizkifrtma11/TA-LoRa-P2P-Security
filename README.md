# 📡 LoRa RA-02 (SX1278) Experiment  
**Sender • Receiver • Attacker (Sniffer) • Security Testing**

Eksperimen ini menunjukkan bagaimana komunikasi LoRa bekerja, serta bagaimana **sniffing (eavesdropping)** dan **replay attack** bisa terjadi jika tidak ada mekanisme keamanan.

---

## 🎯 Tujuan
- Mengirim data antar node LoRa  
- Membuktikan bahwa LoRa default bersifat **broadcast & tidak terenkripsi**  
- Membuat **node attacker (sniffer)** untuk menangkap paket  
- Menguji konsep **replay attack**  
- Membandingkan sebelum & sesudah implementasi security  

---

## 🧰 Hardware
- 2–3x LoRa RA-02 (SX1278)  
- ESP32 / Arduino Uno  
- Regulator 3.3V  
- Antena 433 MHz  
- Kabel jumper  

---

## 🔌 Wiring (ESP32 ↔ LoRa)

| LoRa | ESP32 |
|------|------|
| VCC  | 3.3V |
| GND  | GND  |
| SCK  | GPIO18 |
| MISO | GPIO19 |
| MOSI | GPIO23 |
| NSS  | GPIO5  |
| RST  | GPIO14 |
| DIO0 | GPIO26 |

---

## ⚙️ Parameter LoRa (HARUS SAMA)

```cpp
LoRa.begin(433000000);
LoRa.setSpreadingFactor(7);
LoRa.setSignalBandwidth(125E3);
LoRa.setCodingRate4(5);
```

---

## 🚀 Mode 1 — Sender

```cpp
void loop() {
  LoRa.beginPacket();
  LoRa.print("HELLO FROM ESP32");
  LoRa.endPacket();

  delay(1000);
}
```

---

## 📥 Mode 2 — Receiver

```cpp
void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    Serial.print("Received: ");

    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    Serial.println();
  }
}
```

---

## 👂 Mode 3 — Attacker (Sniffer)

```cpp
void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    Serial.print("[SNIFF] ");

    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    Serial.println();
  }
}
```

---

## 📊 Contoh Output

```
[SNIFF] HELLO FROM ESP32
```

---

## ⚠️ Insight: LoRa Default Tidak Aman

- Tidak ada encryption ❌  
- Tidak ada authentication ❌  
- Semua node bisa mendengar (broadcast)  

---

## 🔁 Replay Attack (Konsep)

1. Attacker menangkap paket  
2. Menyimpan payload  
3. Mengirim ulang paket  

👉 Receiver tetap menerima karena tidak ada proteksi  

---

## 🔐 Security Upgrade

Untuk meningkatkan keamanan:

- AES → enkripsi data  
- HMAC → validasi keaslian  
- Counter / Nonce → anti replay  

Format paket aman:

```
[counter][ciphertext][HMAC]
```

---

## 🧠 Hasil Eksperimen

| Mode | Sniff | Replay |
|------|------|--------|
| Plaintext | ✅ | ✅ |
| Hash saja | ✅ | ✅ |
| AES + HMAC | ❌ | ❌ |
| + Counter | ❌ | ❌ |

---

## 🔥 Tips Debug

- Samakan semua parameter LoRa  
- Gunakan antena  
- Tes jarak dekat  
- Gunakan `LoRa.receive()` untuk sniffer  
- Pastikan supply stabil  

---

## 📌 Catatan

LoRa:
- bersifat broadcast  
- tidak seperti WiFi  
- bisa terjadi packet loss (collision/timing)  

---

## 🚀 Future Work

- Multi-node communication  
- Addressing system  
- Encrypted LoRa  
- Attack simulation  

---

## 👨‍💻 Author

Eksperimen LoRa + Security Testing  