# Purwarupa Sistem Transmisi Gambar VLC-RF Hybrid Berbasis ESP32-CAM & NodeMCU ESP8266

[![Hardware](https://img.shields.io/badge/Hardware-ESP32--CAM%20%7C%20NodeMCU%20ESP8266-blue.svg)](https://www.espressif.com/)
[![Modulation](https://img.shields.io/badge/Modulation-UART%20OOK%2019200%20bps-orange.svg)]()
[![Proximity](https://img.shields.io/badge/Gate-RSSI%20Filtering%20(-45%20dBm)-green.svg)]()
[![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)](LICENSE)

Purwarupa ini merupakan **Sistem Transmisi Gambar Hibrida VLC-RF (Visible Light Communication & Radio-Assisted)** yang dirancang untuk kegiatan demonstrasi edukatif dan Pengabdian Masyarakat (Abdimas). Sistem ini menggabungkan pemancaran data optik tampak melalui LED HPL dengan penyaringan jarak presisi berbasis sinyal radio (ESP-NOW RSSI Gating) untuk menampilkan foto hasil jepretan kamera secara *real-time* pada Web Server lokal.

---

## 📋 Daftar Isi

1. [Fitur Utama](#-fitur-utama)
2. [Prinsip Kerja & Arsitektur Sistem](#-prinsip-kerja--arsitektur-sistem)
3. [Spesifikasi Komponen (BOM)](#-spesifikasi-komponen-bom)
4. [Skematik & Pemetaan Pinout](#-skematik--pemetaan-pinout)
5. [Persiapan Perangkat Lunak](#-persiapan-perangkat-lunak)
6. [Panduan Pengunggahan Program](#-panduan-pengunggahan-program)
7. [Prosedur Pengoperasian & Kalibrasi](#-prosedur-pengoperasian--kalibrasi)
8. [Indikator Kinerja Kanal Optik](#-indikator-kinerja-kanal-optik)
9. [Penanganan Kendala (Troubleshooting)](#-penanganan-kendala-troubleshooting)
10. [Lisensi & Pengembang](#-lisensi--pengembang)

---

## ✨ Fitur Utama

- **Pemotretan Otomatis & Flash Terintegrasi:** ESP32-CAM memanfaatkan LED HPL sebagai penanda *countdown* (3-2-1), lampu *flash* pencahayaan foto, dan pemancar data optik secara bergantian.
- **Enkoding Base64 & Enkapsulasi Paket:** Berkas JPEG dikonversi menjadi *string* Base64 dan dibungkus penanda pengunci `START_B64` (header) serta `~` (footer) untuk integritas data.
- **Sistem Hibrida Radio-Assisted VLC:** Mentransmisikan pulsa bit biner secara optik melalui modulasi Hardware UART $19200\text{ bps}$ sekaligus menyinkronkan *payload* via protokol ESP-NOW jarak pendek.
- **RSSI Proximity Gate (Filter Jarak Otomatis):** Membatasi daya pancar pemancar ($2\text{ dBm}$) dan menyaring sinyal penerima pada ambang batas RSSI (misal $\ge -45\text{ dBm}$) sehingga gambar hanya dapat diterima saat kedua alat diletakkan sejajar dan berdekatan ($\le 15\text{ cm}$).
- **Web Server Lokal & Dashboard Real-Time:** Penerima (ESP8266) memancarkan Access Point mandiri dan menyajikan antarmuka HTML/CSS yang menampilkan gambar JPEG beserta metrik evaluasi kanal optik.

---

## 📐 Prinsip Kerja & Arsitektur Sistem

```
[ ESP32-CAM ] ── (Foto JPEG) ──> [ Base64 Encoder ] ──> [ Header/Footer Wrapper ]
      │                                                         │
      ├── (UART Serial2 @ 19200 bps) ──> [ LED HPL 1W ] ───────┼──> (Kanal Optik)
      │                                                         │
      └── (Low Power 2 dBm ) ──> [ ESP-NOW ] ────┼──> (Kanal RF)
                                                                ▼
[ Web Server Browser ] <── (JPEG Image + Metrics) <── [ ESP8266 Receiver ]
                                                                │
                                                    [ Check RSSI Gate ]
                                                 (Lolos jika RSSI >= -45 dBm)
```

1. **Pemotretan:** ESP32-CAM menangkap gambar JPEG resolusi QQVGA ($160 \times 120\text{ piksel}$).
2. **Pengolahan Data:** Data biner gambar dienkode menjadi deretan karakter ASCII Base64.
3. **Pancaran Optik:** Pin TX Hardware Serial2 memancarkan Bit Start, 8 Bit Data, dan Bit Stop biner Base64 pada laju $19200\text{ bps}$ via LED HPL 1 Watt.
4. **Verifikasi Posisi Fisik:** ESP8266 membaca nilai RSSI dari SSID tersembunyi (`VLC_Beacon`). Jika jarak terlalu jauh ($\text{RSSI} < -45\text{ dBm}$), data ditolak. Jika rapat, data gambar didemodulasi, didekode, dan dirender ke Web Server.

---

## 🛠️ Spesifikasi Komponen 

### Blok Pemancar (Transmitter - ESP32-CAM)
- Mikrokontroler ESP32-S3 / ESP32-CAM (Modul Kamera OV2640/OV3660)
- LED High Power Light (HPL) 1 Watt (Warna Putih)
- Transistor MOSFET IRLZ44N (N-Channel Logic Level Drive)
- Kapasitor Elektrolit $470\ \mu\text{F} / 25\text{V}$ (Buffer Tegangan / Anti-Brownout)
- Resistor: 100 ohm, 330 ohm dan 10k ohm

### Blok Penerima (Receiver - ESP8266)
- Mikrokontroler NodeMCU ESP8266 v3
- Sensor Fotodioda BPW34
- IC Operational Amplifier LM358P
- Resistor: $10\text{ k}\Omega$ (4 unit)
- Kapasitor Keramik $100\text{ nF}$

---

## 🔌 Skematik & Pemetaan Pinout

### 1. Transmitter (ESP32-CAM)

| Komponen / Jalur | Pin ESP32-CAM | Catatan Rangkaian |
| :--- | :--- | :--- |
| **MOSFET Gate** | GPIO 2 (`LED_TX_PIN`) | Seri Resistor $330\ \Omega$ ke Gate + Pull-down $10\text{ k}\Omega$ ke GND |
| **MOSFET Drain** | - | Terhubung ke Kutub Negatif LED HPL 1W |
| **MOSFET Source** | GND | Terhubung ke Ground Utama |
| **Kutub Positif LED HPL**| VCC $5\text{V}$ | Seri Resistor $100\ \Omega$ (Pembatas Arus) |
| **Buffer Elco** | VCC $5\text{V}$ & GND | Paralel $470\ \mu\text{F}$ dekat sumber daya utama |

### 2. Receiver (ESP8266)

| Komponen / Jalur | Pin ESP8266 / Op-Amp | Catatan Rangkaian |
| :--- | :--- | :--- |
| **Fotodioda BPW34** | Pin 2 Op-Amp (`1IN-`) | Pin Inverting LM358 |
| **Pembagi Tegangan** | Pin 3 Op-Amp (`1IN+`) | 2x Resistor 10k ohm antara 3.3V & GND |
| **Feedback Resistor** | Pin 1 ke Pin 2 Op-Amp | Resistor 10k ohm ($R_f$ menghubungkan Output ke Inverting |
| **Output Op-Amp** | Pin 1 LM358 (`1OUT`) | Terhubung ke GPIO 4 (Pin D2 NodeMCU) |
| **Pull-Up Resistor** | Pin D2 ke $3.3\text{ V}$ | Resistor $10\text{ k}\Omega$ untuk *noise immunity* |

---

## 💻 Persiapan Perangkat Lunak

### 1. Konfigurasi Board Manager Arduino IDE
Buka **File > Preferences**, lalu tambahkan URL berikut ke kolom **Additional Boards Manager URLs**:
```text
https://dl.espressif.com/dl/package_esp32_index.json,http://arduino.esp8266.com/stable/package_esp8266com_index.json
```
Instal board berikut melalui **Tools > Board > Boards Manager**:
- **esp32** by Espressif Systems (Versi 2.0.x / 3.x)
- **esp8266** by ESP8266 Community (Versi 3.x)

### 2. Library yang Dibutuhkan
Instal pustaka berikut melalui **Tools > Manage Libraries**:
- `base64` by Densaugeo (Versi 1.3.0 atau terbaru)

---

## 🚀 Panduan Pengunggahan Program

1. **Pengirim (Transmitter):**
   - Buka sketch program transmitter.
   - Set Board: `ESP32S3 Dev Module` atau `AI Thinker ESP32-CAM`.
   - Pastikan variabel `broadcastAddress[]` diisi dengan MAC Address dari ESP8266 Receiver milik Anda.
   - Unggah program melalui kabel USB.

2. **Penerima (Receiver):**
   - Buka sketch program receiver.
   - Set Board: `NodeMCU 1.0 (ESP-12E Module)` / `Generic ESP8266 Module`.
   - Sesuaikan konstanta `RSSI_THRESHOLD` (bawaan: `-45`).
   - Unggah program melalui kabel USB.

---

## 📖 Prosedur Pengoperasian & Kalibrasi

1. **Nyalakan Perangkat:** Berikan daya $5\text{V}$ (via USB/Powerbank) ke kedua mikrokontroler.
2. **Koneksi Jaringan:**
   - Hubungkan HP atau Laptop ke hotspot Wi-Fi yang dipancarkan oleh receiver:
     - **SSID:** `VLC_Receiver_new`
     - **Password:** `123456789`
3. **Buka Antarmuka Web:** Akses alamat IP `http://192.168.4.1` pada peramban web.
4. **Kalibrasi Jarak (RSSI Thresholding):**
   - Dekatkan modul transmitter dan receiver ($\le 10\text{ cm}$) hingga berhadapan lurus.
   - Amati Serial Monitor receiver ($115200\text{ bps}$). Nilai RSSI ideal saat rapat adalah $-30\text{ dBm}$ s.d. $-45\text{ dBm}$.
   - Jika alat dijauhkan ($> 30\text{ cm}$), RSSI akan turun ke $-55\text{ dBm}$ s.d. $-70\text{ dBm}$ dan penerimaan data akan otomatis ditolak.

---

## 📊 Indikator Kinerja Kanal Optik

Dashboard Web Server menampilkan laporan pengukuran kualitas transmisi secara *real-time*:

- **Total Karakter Diterima:** Ukuran total muatan data biner Base64 (bytes).
- **Karakter Error:** Jumlah karakter ilegal yang rusak akibat atenuasi cahaya atau *noise*.
- **Estimasi Error Rate:** Persentase kesalahan bit ($BER$) yang dihitung menggunakan rumus:
  $$\text{BER (\%)} = \left( \frac{\text{Karakter Error}}{\text{Total Karakter Diterima}} \right) \times 100\%$$
- **Durasi Penerimaan:** Waktu tempuh aktual pengiriman data dari byte pertama hingga gambar siap dirender (detik).

---

## ❓ Penanganan Kendala (Troubleshooting)

| Gejala Masalah | Kemungkinan Penyebab | Solusi Teknis |
| :--- | :--- | :--- |
| **Gambar tidak muncul di Web** | Header `START_B64` atau Footer `~` terpotong | Pastikan jarak alat tidak terlalu jauh dan RSSI memenuhi syarat threshold. |
| **Pesan Serial `[DITOLAK]` muncul terus** | Nilai di bawah threshold saat alat rapat | Longgarkan `RSSI_THRESHOLD` pada ESP8266 dari `-45` ke `-50` atau `-55`. |
| **Karakter acak pada Serial Receiver** | Baud rate tidak cocok / Logika terbalik | Samakan baud rate ($19200\text{ bps}$) dan set `SoftwareSerial vlcSerial(4, -1, false);`. |
| **ESP32-CAM *Restart* saat Flash** | Lonjakan arus LED HPL (*Brownout*) | Pastikan Kapasitor $470\ \mu\text{F}$ terpasang rapat di jalur daya $5\text{V}$ & GND ESP32. |

---

## 📜 Lisensi & Pengembang

Proyek ini dikembangkan sebagai bagian dari kegiatan **Pengabdian Kepada Masyarakat (Abdimas)** untuk edukasi teknologi telekomunikasi optik hibrida.

Distributed under the **MIT License**. See `LICENSE` for more information.
