#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <espnow.h>

const char* ap_ssid = "VLC_Receiver_new";      
const char* ap_password = "123456789";     

// BATAS AMBANG RSSI
const int RSSI_THRESHOLD = -55; 

ESP8266WebServer server(80);

uint8_t imageBuffer[15000];
int imageSize = 0;
bool newImage = false;
String incomingB64 = "";

unsigned long totalReceivedChars = 0;
unsigned long errorCharsCount = 0;
float lastErrorRate = 0.0; 

unsigned long rxStartTime = 0;
float rxDuration = 0.0;
unsigned long lastImageTime = 0;

typedef struct struct_message {
  char chunk[200];
  bool isEnd;
} struct_message;

struct_message myData;

int decodeBase64(const char* input, int inputLen, uint8_t* output) {
  const char* b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int outLen = 0;
  uint32_t buf = 0;
  int bits = 0;
  for (int i = 0; i < inputLen; i++) {
    char c = input[i];
    if (c == '=') break;
    const char* p = strchr(b64chars, c);
    if (!p) continue;
    buf = (buf << 6) | (p - b64chars);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      output[outLen++] = (buf >> bits) & 0xFF;
    }
  }
  return outLen;
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  int currentRSSI = WiFi.RSSI();

  if (currentRSSI == 0 || currentRSSI < RSSI_THRESHOLD) {
    Serial.printf("\n[DITOLAK] Terlalu Jauh! RSSI: %d dBm (Butuh >= %d dBm)", currentRSSI, RSSI_THRESHOLD);
    incomingB64 = ""; 
    return; 
  }

  memcpy(&myData, incomingData, sizeof(myData));

  // 1. Deteksi Header START_B64 (Abaikan teks ini agar tidak merusak Base64)
  if (strcmp(myData.chunk, "START_B64") == 0) {
    incomingB64 = ""; 
    rxStartTime = millis();
    Serial.printf("\n[PROSES] Pengiriman Data Gambar .\n", currentRSSI);
    Serial.println("--- MEMULAI PENERIMAAN STRUKTUR BASE64--");
    return;
  }

  // 2. Deteksi Footer Penutup ~ atau status isEnd
  if (strcmp(myData.chunk, "~") == 0 || myData.isEnd) {
    Serial.println("\n--- AKHIR DATA BASE64 ---");
    rxDuration = (millis() - rxStartTime) / 1000.0;
    totalReceivedChars = incomingB64.length();
    errorCharsCount = 0;
    lastErrorRate = 0.0;

    Serial.println("\n[SELESAI] Data gambar Base64 utuh berhasil diterima!");

    int inputLen = incomingB64.length();
    imageSize = decodeBase64(incomingB64.c_str(), inputLen, imageBuffer);

    newImage = true;
    lastImageTime = millis(); 
    incomingB64 = ""; 
    return;
  }

  // 3. Simpan hanya potongan data murni Base64
  incomingB64 += String(myData.chunk);
  Serial.print(myData.chunk);
}

void handleRoot() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent(F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta charset='UTF-8'><title>Penerima Gambar VLC</title><style>body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #f4f7f6; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; min-height: 100vh; }.card { background: #ffffff; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.05); padding: 40px; width: 90%; max-width: 500px; text-align: center; box-sizing: border-box; }h1 { color: #007afc; font-size: 24px; font-weight: 700; margin: 0 0 8px 0; }h2 { color: #6c757d; font-size: 14px; font-weight: 400; margin: 0 0 24px 0; }.status-badge { display: inline-flex; align-items: center; background-color: #ffebeb; color: #cc3333; font-size: 13px; font-weight: 600; padding: 6px 16px; border-radius: 20px; margin-bottom: 24px; transition: all 0.3s ease; }.status-badge.connected { background-color: #e6f9ed; color: #1e7e34; }.dot { height: 8px; width: 8px; border-radius: 50%; display: inline-block; margin-right: 8px; background-color: #cc3333; }.status-badge.connected .dot { background-color: #1e7e34; }.image-container { border: 2px dashed #cccccc; border-radius: 8px; padding: 20px; min-height: 200px; display: flex; flex-direction: column; justify-content: center; align-items: center; background-color: #fafafa; margin-bottom: 24px; overflow: hidden; }.image-container img { max-width: 100%; height: auto; border-radius: 6px; box-shadow: 0 4px 12px rgba(0,0,0,0.08); }.placeholder-text { color: #aaaaaa; font-size: 14px; }.btn { background-color: #007afc; color: white; border: none; padding: 14px 28px; font-size: 15px; font-weight: 600; border-radius: 8px; cursor: pointer; width: 100%; display: flex; justify-content: center; align-items: center; gap: 8px; transition: background-color 0.2s ease; margin-top: 20px; }.btn:hover { background-color: #0062cc; }</style></head><body>"));

  server.sendContent(F("<div class='card'><h1>Penerima Gambar VLC</h1><h2></h2><div id='status' class='status-badge'><span class='dot'></span><span id='status-text'>Terputus dari ESP8266</span></div><div class='image-container'>"));

  if (newImage) {
    String htmlImg = "<img src='/image.jpg?t=" + String(millis()) + "' alt='VLC Image'>";

    htmlImg += "<div style='text-align:left; background:#eef2f5; padding:15px; border-radius:8px; margin-top:20px; width:100%; box-sizing:border-box;'>";
    htmlImg += "<h3 style='font-size:15px; margin-top:0; color:#007afc; border-bottom:2px solid #ddd; padding-bottom:8px; text-align:center;'>LAPORAN KUALITAS KANAL OPTIK (VLC)</h3>";
    htmlImg += "<p style='margin:8px 0 4px 0; font-size:14px; color:#444;'><b>Total Karakter Diterima :</b> " + String(totalReceivedChars) + " bytes</p>";
    htmlImg += "<p style='margin:4px 0; font-size:14px; color:#444;'><b>Karakter Error :</b> " + String(errorCharsCount) + " bytes</p>";

    if (lastErrorRate > 0) {
        htmlImg += "<p style='margin:4px 0; font-size:14px; color:#cc3333;'><b>Estimasi Error Rate :</b> " + String(lastErrorRate, 4) + " %</p>";
    } else {
        htmlImg += "<p style='margin:4px 0; font-size:14px; color:#1e7e34;'><b>Estimasi Error Rate :</b> 0.0000 %</p>";
    }

    htmlImg += "<p style='margin:4px 0; font-size:14px; color:#444;'><b>Durasi Penerimaan :</b> " + String(rxDuration, 2) + " detik</p>";
    htmlImg += "</div>";

    server.sendContent(htmlImg);
  } else {
    server.sendContent(F("<p class='placeholder-text'>Menunggu data transmisi cahaya...</p>"));
  }

  server.sendContent(F("</div><button class='btn' onclick='location.reload()'>🔄 Muat Ulang Manual</button></div>"));

  server.sendContent(F("<script>"));
  server.sendContent(F("let currentImgTime = '0';"));
  server.sendContent(F("function checkStatus() {"));
  server.sendContent(F("  fetch('/status-ping').then(response => { if(response.ok) { document.getElementById('status').className = 'status-badge connected'; document.getElementById('status-text').innerText = 'Terhubung ke ESP8266'; } }).catch(() => { document.getElementById('status').className = 'status-badge'; document.getElementById('status-text').innerText = 'Terputus dari ESP8266'; });"));
  server.sendContent(F("  fetch('/check-update').then(response => response.text()).then(text => {"));
  server.sendContent(F("    if (currentImgTime === '0') { currentImgTime = text; }"));
  server.sendContent(F("    else if (text !== currentImgTime) { location.reload(); }")); 
  server.sendContent(F("  });"));
  server.sendContent(F("}"));
  server.sendContent(F("setInterval(checkStatus, 2000); window.onload = checkStatus;"));
  server.sendContent(F("</script></body></html>"));

  server.sendContent(""); 
}

void handleImg() {
  server.setContentLength(imageSize);
  server.send(200, "image/jpeg", ""); 
  WiFiClient client = server.client();
  client.write(imageBuffer, imageSize); 
}

void setup() {
  Serial.begin(115200);
  incomingB64.reserve(20000); 

  WiFi.mode(WIFI_AP_STA); 
  WiFi.softAP(ap_ssid, ap_password);

  WiFi.begin("VLC_Beacon", "12345678");

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Hotspot Aktif! Hubungkan laptop/HP ke WiFi: ");
  Serial.println(ap_ssid);
  Serial.print("Buka browser dan akses IP: ");
  Serial.println(myIP); 

  if (esp_now_init() != 0) {
    Serial.println("Gagal Inisialisasi ESP");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  server.on("/", handleRoot);
  server.on("/image.jpg", handleImg);
  server.on("/status-ping", []() { server.send(200, "text/plain", "OK"); });
  server.on("/check-update", []() { server.send(200, "text/plain", String(lastImageTime)); });

  server.begin();
  Serial.println("Sistem Receiver Siap!");
}

void loop() {
  server.handleClient();
}