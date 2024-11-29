#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <Adafruit_MCP4725.h>
#include "config.h"
#include "AudioManager.h"
#include "FileManager.h"
#include "TimeManager.h"
#include "WifiManager.h"
#include "MQTTManager.h"
#include "WebServer.h"
#include "BluetoothManager.h"

// Dosyanın başına, include'lardan sonra ekleyin
void printDirectory(File dir, int numTabs);  // Fonksiyon prototipi

// Global nesneler
RTC_DS3231 rtc;
Adafruit_MCP4725 dac;
AsyncWebServer server(80);

// Yönetici sınıfları
AudioManager audioManager(dac);
FileManager fileManager;
TimeManager timeManager(rtc);
MQTTManager mqttManager(audioManager, timeManager);
WifiManager wifiManager(server);
BluetoothManager bleManager(audioManager, timeManager);
WebServer webServer(server, audioManager, fileManager, timeManager, mqttManager, bleManager);

// Sistem durumu
SystemStatus systemStatus = {
    .isPlaying = false,
    .volume = DEFAULT_VOLUME,
    .isWifiConnected = false,
    .isMqttConnected = false,
    .isSdCardPresent = false,
    .isRtcPresent = false,
    .lastError = ERR_NONE
};

// Durum LED'i kontrolü
void setStatusLED(uint8_t r, uint8_t g, uint8_t b) {
    analogWrite(LED_R, r);
    analogWrite(LED_G, g);
    analogWrite(LED_B, b);
}

// I2C cihazlarını başlat
bool initI2CDevices() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // DAC başlatma
    if (!dac.begin(MCP4725_ADDR)) {
        systemStatus.lastError = ERR_DAC_INIT;
        return false;
    }
    
    // RTC başlatma
    if (!rtc.begin()) {
        systemStatus.lastError = ERR_RTC_INIT;
        return false;
    }
    
    systemStatus.isRtcPresent = true;
    return true;
}

void setup() {
    // Seri port başlatma
    Serial.begin(115200);
    delay(1000);  // Seri port stabilizasyonu için bekle
    
    Serial.println("\n=== ESP32 Music Box Starting ===");
    Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION);
    
    // GPIO pinlerini ayarla
    pinMode(LED_R, OUTPUT);
    pinMode(LED_G, OUTPUT);
    pinMode(LED_B, OUTPUT);
    
    // Başlangıç durumu - Mavi yanıp sönme
    setStatusLED(0, 0, 255);
    
    // WiFi Manager'ı ilk başlat
    Serial.println("\n=== Initializing WiFi ===");
    if (!wifiManager.begin()) {
        Serial.println("❌ WiFi initialization failed!");
        setStatusLED(255, 0, 0);  // Kırmızı LED
        return;  // WiFi olmadan devam etme
    }
    Serial.println("✅ WiFi Manager initialized");
    
    // WiFi bağlantısını bekle
    delay(1000);
    
    // Web Server'ı başlat
    Serial.println("\n=== Initializing Web Server ===");
    if (!webServer.begin()) {
        Serial.println("❌ Failed to initialize Web Server!");
        setStatusLED(255, 0, 0);
        return;
    }
    Serial.println("✅ Web Server initialized");
    
    // I2C cihazlarını başlat
    Serial.println("\n=== Checking I2C Devices ===");
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);
    
    // DAC kontrolü
    if (!dac.begin(MCP4725_ADDR)) {
        Serial.println("❌ MCP4725 DAC not found!");
        setStatusLED(255, 0, 0);
        return;
    }
    Serial.println("✅ MCP4725 DAC initialized");
    
    // RTC kontrolü
    if (!rtc.begin()) {
        Serial.println("❌ RTC not found!");
        setStatusLED(255, 0, 0);
        return;
    }
    Serial.println("✅ RTC initialized");
    
    if (rtc.lostPower()) {
        Serial.println("⚠️ RTC lost power, setting time to compile time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    // SD kart kontrolü
    Serial.println("\n=== Checking SD Card ===");
    if (!fileManager.begin()) {
        Serial.println("❌ Failed to initialize SD Card!");
        setStatusLED(255, 0, 0);  // Kırmızı LED
        // return;  // Bu satırı yorum yapın - SD kart olmadan da devam etsin
    }
    
    // SD kart durumunu göster
    if (fileManager.isSDCardMounted()) {
        Serial.println("✅ SD Card mounted successfully");
        Serial.printf("Music Files: %d\n", fileManager.getMusicFileCount());
    } else {
        Serial.println("⚠️ SD Card not mounted - continuing without SD card");
    }
    
    // SPIFFS içeriğini kontrol et
    Serial.println("\n=== Checking SPIFFS ===");
    if (SPIFFS.begin(true)) {
        Serial.println("✅ SPIFFS mounted successfully");
        FileManager::listSPIFFSContents();
    } else {
        Serial.println("❌ SPIFFS mount failed!");
    }
    
    // SPIFFS kontrolü
    if(!SPIFFS.begin(true)) {
        Serial.println("❌ SPIFFS Mount Failed");
        setStatusLED(255, 0, 0);  // Kırmızı LED
        return;
    }
    
    Serial.println("\n=== SPIFFS Check ===");
    Serial.printf("Total space: %u bytes\n", SPIFFS.totalBytes());
    Serial.printf("Used space: %u bytes\n", SPIFFS.usedBytes());
    
    // Gerekli dosyaları kontrol et
    if(!SPIFFS.exists("/index.html")) {
        Serial.println("❌ index.html not found!");
        setStatusLED(255, 0, 0);
        return;
    }
    if(!SPIFFS.exists("/css/style.css")) {
        Serial.println("❌ style.css not found!");
        setStatusLED(255, 0, 0);
        return;
    }
    if(!SPIFFS.exists("/js/app.js")) {
        Serial.println("❌ app.js not found!");
        setStatusLED(255, 0, 0);
        return;
    }
    
    Serial.println("✅ All required files found in SPIFFS");
    
    // Dosyaların içeriğini kontrol et
    File file = SPIFFS.open("/index.html", "r");
    if(file) {
        Serial.println("\nindex.html content:");
        while(file.available()) {
            Serial.write(file.read());
        }
        file.close();
    }
    
    // Yönetici sınıflarını başlat
    Serial.println("\n=== Initializing Managers ===");
    
    if (!fileManager.begin()) {
        Serial.println("❌ Failed to initialize File Manager!");
        setStatusLED(255, 0, 0);
        return;
    }
    Serial.println("✅ File Manager initialized");
    
    if (!audioManager.begin()) {
        Serial.println("❌ Failed to initialize Audio Manager!");
        setStatusLED(255, 0, 0);
        return;
    }
    Serial.println("✅ Audio Manager initialized");
    
    if (!timeManager.begin()) {
        Serial.println("❌ Failed to initialize Time Manager!");
        setStatusLED(255, 0, 0);
        return;
    }
    Serial.println("✅ Time Manager initialized");
    
    if (!webServer.begin()) {
        Serial.println("❌ Failed to initialize Web Server!");
        setStatusLED(255, 0, 0);
        return;
    }
    Serial.println("✅ Web Server initialized");
    
    if (!mqttManager.begin()) {
        Serial.println("⚠️ Failed to initialize MQTT Manager");
        // MQTT hatası kritik değil
    } else {
        Serial.println("✅ MQTT Manager initialized");
    }
    
    // BluetoothManager başlatmasını kaldır veya koşullu hale getir
    #ifdef ENABLE_BLUETOOTH
        if (!bleManager.begin()) {
            Serial.println("❌ Failed to initialize Bluetooth!");
            setStatusLED(255, 0, 0);
            return;
        }
        Serial.println("✅ Bluetooth initialized");
    #endif
    
    // Başarılı başlatma - Yeşil LED
    setStatusLED(0, 255, 0);
    Serial.println("\n=== Initialization Complete ===\n");
}

void loop() {
    // Tüm yönetici sınıfların loop fonksiyonlarını çağır
    audioManager.loop();
    timeManager.loop();
    wifiManager.loop();
    webServer.loop();
    mqttManager.loop();
    bleManager.loop();
    
    // Her 5 saniyede bir sistem durumunu güncelle
    static uint32_t lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate >= 5000) {
        lastStatusUpdate = millis();
        
        // Durum güncellemeleri
        systemStatus.volume = audioManager.getVolume();  // Volume'u AudioManager'dan al
        systemStatus.isPlaying = audioManager.isCurrentlyPlaying();  // Playing durumunu güncelle
        
        Serial.println("\n=== System Status ===");
        
        // WiFi ve MQTT durumlarını güncelle
        systemStatus.isWifiConnected = wifiManager.isWiFiConnected();
        systemStatus.isMqttConnected = mqttManager.isConnectedToMqtt();
        
        // RTC durumu
        if (systemStatus.isRtcPresent) {
            DateTime now = timeManager.getDateTime();
            Serial.printf("Time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
            Serial.printf("Temperature: %.2f°C\n", timeManager.getTemperature());
        }
        
        // SD kart durumu
        if (fileManager.isSDCardMounted()) {
            Serial.println("SD Card: Connected");
            Serial.printf("Music Files: %d\n", fileManager.getMusicFileCount());
        } else {
            Serial.println("SD Card: Not Connected");
        }
        
        // Ses durumu
        Serial.printf("Volume: %d%%\n", systemStatus.volume);
        Serial.printf("Playing: %s\n", systemStatus.isPlaying ? "Yes" : "No");
        
        // Ağ durumu
        Serial.printf("WiFi: %s\n", systemStatus.isWifiConnected ? "Connected" : "Disconnected");
        Serial.printf("MQTT: %s\n", systemStatus.isMqttConnected ? "Connected" : "Disconnected");
        
        Serial.println("==================\n");
    }
    
    // Watchdog yenileme
    delay(10);
}

// printDirectory fonksiyonunu main.cpp'ye ekleyin
void printDirectory(File dir, int numTabs) {
    while (File entry = dir.openNextFile()) {
        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }
        String fileName = entry.name();
        Serial.print(fileName);
        if (entry.isDirectory()) {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        } else {
            // MP3 dosyalarını özel olarak işaretle
            if (fileName.endsWith(".mp3")) {
                Serial.print(" [MUSIC]");
            }
            Serial.printf("\t%d bytes\n", entry.size());
        }
        entry.close();
    }
} 