# 📡 LoRa RA-02 (SX1278) Experiment  
**Sender • Receiver • Attacker (Sniffer) • Security Testing**

Eksperimen ini menunjukkan bagaimana komunikasi LoRa bekerja pada lapisan Physical dan pada jaringan Peer-to-Peer, serta bagaimana **sniffing (eavesdropping)** dan **replay attack** bisa terjadi jika tidak ada mekanisme keamanan.

---

## 🎯 Tujuan
- Mengirim data antar node LoRa  
- Membuktikan bahwa LoRa default bersifat **broadcast & tidak terenkripsi**  
- Membuat **node attacker (sniffer)** untuk menangkap paket  
- Menguji konsep **replay attack**  
- Membandingkan sebelum & sesudah implementasi security  

---

## 🧰 Hardware
- 3x LoRa RA-02 (SX1278)  
- 3x ESP32
- 3x Regulator AMS1117 3.3V  
- 3x Antena 433 MHz
- Kabel jumper
- 3x Project Board
- DHT22

---

## 📊 Contoh Output

Attacker:
```
[SNIFF] HELLO FROM ESP32
```
atau
```
[TAMPER] HELLO FROM ESP32
```
Sender:
```
[SENDER] HELLO FROM ESP32
```
Receiver:
```
[RECEIVER] HELLO FROM ESP32
```
---

## ⚠️ Insight: LoRa Default Tidak Aman

- Tidak ada encryption ❌  
- Tidak ada authentication ❌  
- Node yang mengetahui konfigurasi komunikasi bisa mendengar (broadcast)  

---

## 🔁 Replay Attack (Konsep)

1. Attacker menangkap paket  
2. Menyimpan payload  
3. Mengirim ulang paket

## 

👉 Receiver tetap menerima karena tidak ada proteksi

---

## 🔐 Security Upgrade

Untuk meningkatkan keamanan:

- AES → enkripsi data  
- HMAC → validasi keaslian  
- Counter-based Nonce → anti replay  

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

**Analisis Konsumsi Daya (*Power Profiling*):** Peneliti selanjutnya disarankan untuk melakukan pengukuran konsumsi daya secara langsung menggunakan alat ukur untuk mengetahui dampak penambahan beban siklus komputasi kriptografi (AES, HMAC, KDF) terhadap baterai perangkat *End-Node* (dalam satuan miliampere atau Joule). Hal ini penting agar estimasi masa pakai (*lifetime*) baterai untuk implementasi jaringan *Internet of Things* (IoT) jangka panjang dapat dipastikan secara optimal.

---

## 👨‍💻 Author
Mohammad Rizki Fadillah

## 📝 Lisensi
Proyek ini dibuat untuk keperluan akademis di Program Studi Teknik Multimedia dan Jaringan, Jurusan Teknik Informatika dan Komputer, Politeknik Negeri Jakarta (PNJ). Penggunaan atau modifikasi kode diharapkan tetap menyertakan referensi ke repositori ini.
