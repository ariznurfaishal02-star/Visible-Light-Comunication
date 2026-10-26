#include "esp_camera.h"
#include "base64.h"
#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

#define CAMERA_MODEL_ESP32S3_EYE 
#include "camera_pins.h"

#define LED_TX_PIN 2

uint8_t broadcastAddress[] = {0x8C, 0xAA, 0xB5, 0xCF, 0x80, 0x74}; 

typedef struct struct_message {
  char chunk[200];
  bool isEnd;
} struct_message;

struct_message myData;

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_TX_PIN, OUTPUT);
  digitalWrite(LED_TX_PIN, LOW);

  // Inisialisasi Kamera
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
  config.jpeg_quality = 12; 
  config.fb_count     = 1;

  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_camera_init(&config);
  
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_vflip(s, 1);   
    s->set_hmirror(s, 1); 
  }

  // Inisialisasi Wi-Fi & ESP-NOW (Hidden AP)
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("VLC_Beacon", "12345678", 1, 1); 
  
  esp_wifi_set_max_tx_power(8); // Daya Wi-Fi minim (2 dBm)

  if (esp_now_init() == ESP_OK) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }

  Serial.println("ESP32-CAM Transmitter Siap!");
  delay(1000);
}

void loop() {
  Serial.println("\n=======================================================");
  Serial.println("MEMULAI SIKLUS PENGAMBILAN & PENGIRIMAN GAMBAR");
  Serial.println("=======================================================");
  
  pinMode(LED_TX_PIN, OUTPUT);
  digitalWrite(LED_TX_PIN, LOW);

  // TAHAP 1: Hitung mundur kedip 3, 2, 1
  Serial.println("[1] Bersiap difoto dalam 3 detik...");
  for (int i = 3; i > 0; i--) {
    Serial.printf("    %d...\n", i);
    digitalWrite(LED_TX_PIN, HIGH); 
    delay(200);                     
    digitalWrite(LED_TX_PIN, LOW);  
    delay(800);                     
  }

  // TAHAP 2: Nyalakan Lampu Flash & Ambil Foto
  Serial.println("[2] CEKREK! Mengambil foto...");
  digitalWrite(LED_TX_PIN, HIGH); 
  delay(300); 

  camera_fb_t *dummy_fb = esp_camera_fb_get();
  if (dummy_fb) esp_camera_fb_return(dummy_fb); 

  camera_fb_t *fb = esp_camera_fb_get();
  digitalWrite(LED_TX_PIN, LOW);

  if (!fb) {
    Serial.println("Gagal mengambil foto! Mengulang siklus...");
    delay(2000);
    return;
  }

  String base64Image = base64::encode(fb->buf, fb->len);
  int totalLen = base64Image.length();
  esp_camera_fb_return(fb);

  // TAHAP 3: Transmisi Data
  Serial.println("[3] Memulai transmisi (UART 19200 bps & ESP-NOW)...");
  
  // Aktifkan Hardware Serial2 di Pin GPIO 2
  Serial2.begin(19200, SERIAL_8N1, -1, LED_TX_PIN);

  // 1. KIRIM HEADER PENGUNCI "START_B64"
  myData.isEnd = false;
  strcpy(myData.chunk, "START_B64");
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  Serial2.print("START_B64");
  Serial2.flush();
  delay(100);

  // 2. KIRIM CHUNK DATA BASE64
  int chunkSize = 190;
  int tenPercent = totalLen / 10;
  if (tenPercent == 0) tenPercent = 1;

  for (int i = 0; i < totalLen; i += chunkSize) {
    String sub = base64Image.substring(i, i + chunkSize);
    sub.toCharArray(myData.chunk, 200);
    myData.isEnd = false; 

    // Kirim via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    // Kirim via Optik LED HPL
    Serial2.print(sub);
    Serial2.flush(); 

    if (i % tenPercent == 0 && i != 0) {
      Serial.printf("    Progres Transmisi: %d%%\n", (i / tenPercent) * 10);
    }
  }

  // 3. KIRIM FOOTER PENUTUP "~"
  myData.isEnd = true;
  strcpy(myData.chunk, "~");
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  Serial2.print("~");
  Serial2.flush();
  delay(100);

  // Matikan Serial2 agar LED padam saat menganggur
  Serial2.end();
  pinMode(LED_TX_PIN, OUTPUT);
  digitalWrite(LED_TX_PIN, LOW);

  Serial.println("[4] Transmisi 100% Selesai!");
  Serial.println("Menunggu 8 detik sebelum siklus berikutnya...\n");
  delay(8000); 
}