#include "esp_camera.h"
#include "base64.h"

// model kamera yang digunakan
#define CAMERA_MODEL_ESP32S3_EYE 
#include "camera_pins.h"

// Konfigurasi Pin dan Kecepatan Komunikasi
#define LED_TX_PIN 2
#define BAUD_RATE 19200

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  if(psramFound()){
    Serial.println("PSRAM Siap!");
  } else {
    Serial.println("PERINGATAN: PSRAM tidak ditemukan!");
  }

  // Konfigurasi Parameter Kamera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size   = FRAMESIZE_QQVGA; 
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode    = CAMERA_GRAB_LATEST;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12; 
  config.fb_count     = 1;

  // Inisialisasi Hardware Kamera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Inisialisasi kamera gagal dengan error 0x%x\n", err);
    return;
  }
  
  // ROTASI 180 DERAJAT 
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);   // Putar atas ke bawah
  s->set_hmirror(s, 1); // Balik kiri ke kanan
  
  Serial.println("Kamera berhasil diinisialisasi dan dirotasi 180 derajat.");
  Serial.println("Sistem Otomatis akan segera dimulai...\n");
  delay(2000);
}

void loop() {
  Serial.println("\n=======================================================");
  Serial.println("MEMULAI SIKLUS BARU");
  Serial.println("=======================================================");
  
  
  // Aba-aba Kedip 3 Detik
  Serial.println("[1] Bersiap difoto dalam 3 detik...");
  
  // Matikan fungsi UART sementara agar LED bisa dibuat berkedip manual
  Serial1.end(); 
  pinMode(LED_TX_PIN, OUTPUT);

  // Kedip hitung mundur
  for (int i = 3; i > 0; i--) {
    Serial.printf("    %d...\n", i);
    digitalWrite(LED_TX_PIN, HIGH); // Flash peringatan
    delay(150);                     
    digitalWrite(LED_TX_PIN, LOW);  
    delay(850);                     
  }

  // Jepret Foto (Teknik Flushing agar tidak telat 1 frame)
  Serial.println("[2] CEKREK! Mengambil foto...");
  
  // 2A. Ambil dan buang gambar sebelumnya di memori buffer
  camera_fb_t* dummy_fb = esp_camera_fb_get();
  if (dummy_fb) {
    esp_camera_fb_return(dummy_fb); 
  }
  
  
  // memberi waktu pada Auto Exposure (AEC) kamera untuk menstabilkan 
  // cahaya ekstrem sebelum mengambil foto yang sebenarnya.
  delay(800); 

  // 2B. Ambil gambar aktual 
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Gagal mengambil foto! Mengulang siklus...");
    delay(2000);
    return;
  }
  Serial.printf("    Foto berhasil diambil! Ukuran: %d bytes\n", fb->len);

  
  // Jeda Aman 4 Detik (Sambil Konversi Base64)
  Serial.println("[3] Jeda 4 detik....");
  
  // Lakukan komputasi encode selama masa jeda aman
  String base64Image = base64::encode(fb->buf, fb->len);
  Serial.printf("    Data Base64 siap: %d karakter\n", base64Image.length());
  
  delay(4000); 


  // TAHAP 4: Transmisi via LED VLC
  Serial.println("[4] Memulai transmisi cahaya berkecepatan tinggi (VLC)...");

  // Aktifkan kembali fungsi UART (TX) ke LED
  Serial1.begin(BAUD_RATE, SERIAL_8N1, -1, LED_TX_PIN, true);
  delay(100); 

  // Kirim Header
  Serial1.print("START_B64");
  delay(100);

  // Kalkulasi untuk Progress Bar
  int totalLen = base64Image.length();
  int tenPercent = totalLen / 10;

  // Proses pengiriman karakter
  for (int i = 0; i < totalLen; i++) {
    Serial1.write(base64Image[i]);
    
    // Tampilkan progres
    if (i % tenPercent == 0 && i != 0) {
      Serial.printf("    Progres Transmisi: %d%%\n", (i / tenPercent) * 10);
    }
  }

  // Kirim Footer
  Serial1.print("~");
  Serial1.flush(); 

  // Bebaskan memori gambar agar ESP tidak kehabisan RAM
  esp_camera_fb_return(fb);
  
  Serial.println("[4] Transmisi 100% selesai!");
  
  // TAHAP 5: Istirahat Antar Siklus
  Serial.println("\n[5] Menunggu 10 detik sebelum mengulang siklus berikutnya...");
  delay(10000); 
}