# 📡 LoRa RA-02 (SX1278) Experiment  
**Sender • Receiver • Attacker (Sniffer) • Security Testing**

Eksperimen ini menunjukkan bagaimana komunikasi LoRa bekerja, serta bagaimana **sniffing (eavesdropping)** dan **replay attack** bisa terjadi jika tidak ada mekanisme keamanan.

---

## 🎯 Tujuan
- Mengirim data antar node LoRa
- Membuktikan bahwa LoRa default bersifat **broadcast & tidak terenkripsi**
- Membuat **node attacker (sniffer)** untuk menangkap paket
- Menguji konsep **replay attack**
- Membandingkan sebelum & sesudah implementasi security (AES, HMAC, counter)

---

## 🧰 Hardware
- 2–3x LoRa RA-02 (SX1278)
- ESP32 / Arduino Uno
- Regulator 3.3V (wajib untuk RA-02)
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