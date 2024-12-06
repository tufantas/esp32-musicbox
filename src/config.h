#ifndef CONFIG_H
#define CONFIG_H

// Version
#define FIRMWARE_VERSION "1.0.0"

// Pin Definitions
// I2C
#define I2C_SDA 21
#define I2C_SCL 16

// SD Card
#define SD_CS    5
#define SD_MOSI  23
#define SD_MISO  19
#define SD_SCK   18

// Status LED
#define LED_R 32
#define LED_G 33
#define LED_B 27

// I2C Addresses
#define RTC_ADDR     0x68

// MQTT Settings
#define MQTT_LOCAL_BROKER  true
#define MQTT_HOST         "192.168.1.100"  // Local Mosquitto server
#define MQTT_PORT         1883
#define STATUS_UPDATE_INTERVAL 5000  // 5 saniye
#define MQTT_LOCAL_ONLY   true

// MQTT Topics
#define TOPIC_COMMAND  "musicbox/command"
#define TOPIC_VOLUME   "musicbox/volume"
#define TOPIC_STATUS   "musicbox/status"
#define TOPIC_PLAYLIST "musicbox/playlist"
#define TOPIC_TIMER    "musicbox/timer"
#define TOPIC_SYSTEM   "musicbox/system"

// WiFi Settings
#define WIFI_AP_SSID "ESP32_MusicBox_Setup"
#define WIFI_CONNECT_TIMEOUT 60000    // 60 saniye
#define WIFI_CONNECT_DELAY 1000       // 1 saniye
#define WIFI_MAX_RETRY 5              // Maksimum yeniden deneme
#define WIFI_POWER_SAVE false         // Güç tasarrufu kapalı
#define WIFI_TX_POWER WIFI_POWER_19_5dBm  // WiFi sinyal gücü maksimum

// Web Server
#define WEB_SERVER_PORT 80

// Audio Settings
#define DEFAULT_VOLUME 50
#define MAX_VOLUME 100
#define FADE_STEP 5
#define FADE_DELAY 50

// System Settings
#define WDT_TIMEOUT 120000  // 120 saniye
#define MAX_RECONNECT_ATTEMPTS 3
#define RECONNECT_DELAY 10000

// Debug Settings
#define ENABLE_SERIAL_DEBUG false  // Seri port debug mesajlarını aç/kapa

#if ENABLE_SERIAL_DEBUG
    #define DEBUG_BEGIN(x)     Serial.begin(x)
    #define DEBUG_PRINT(x)     Serial.print(x)
    #define DEBUG_PRINTLN(x)   Serial.println(x)
    #define DEBUG_PRINTF(x...) Serial.printf(x)
#else
    #define DEBUG_BEGIN(x)
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(x...)
#endif

// Error Codes
enum ErrorCode {
    ERR_NONE = 0,
    ERR_SD_INIT = 1,
    ERR_WIFI_CONNECT = 2,
    ERR_MQTT_CONNECT = 3,
    ERR_RTC_INIT = 5,
    ERR_FILE_OPEN = 6,
    ERR_FILE_READ = 7,
    ERR_FILE_WRITE = 8
};

// System Status
struct SystemStatus {
    bool isPlaying;
    int volume;
    bool isWifiConnected;
    bool isMqttConnected;
    bool isSdCardPresent;
    bool isRtcPresent;
    ErrorCode lastError;
};

// Status Messages
#define STATUS_WIFI_CONNECTED "✅ WiFi Connected: %s"
#define STATUS_WIFI_DISCONNECTED "❌ WiFi Disconnected"
#define STATUS_MUSIC_FILES "Music Files: %d"
#define STATUS_VOLUME "Volume: %d%%"
#define STATUS_PLAYING "Playing: %s"
#define STATUS_STOPPED "Stopped"

// PCM5102 Pin Definitions
#define I2S_BCLK 26 // BCK pin
#define I2S_LRCK 25 // LCK pin
#define I2S_DOUT 22 // DIN pin

// Audio Buffer Settings
#define AUDIO_BUFFER_SIZE 8192
#define AUDIO_BASE_PRIORITY 3  // Temel öncelik

// Task Priorities (Audio base priority üzerine eklenir)
#define WIFI_TASK_PRIORITY (AUDIO_BASE_PRIORITY + 2)    // 5
#define AUDIO_TASK_PRIORITY (AUDIO_BASE_PRIORITY + 1)   // 4
#define WEB_TASK_PRIORITY AUDIO_BASE_PRIORITY           // 3
#define MQTT_TASK_PRIORITY (AUDIO_BASE_PRIORITY - 1)    // 2

// I2S Configuration
#define I2S_PORT I2S_NUM_0
#define I2S_DMA_BUFFER_COUNT 8
#define I2S_DMA_BUFFER_LEN 1024

// Power Management Settings
#define POWER_CHECK_INTERVAL 5000     // Güç kontrolü aralığı
#define VOLTAGE_THRESHOLD 3.0         // Minimum voltaj eşiği
#define CPU_FREQ_NORMAL 240     // Normal çalışma frekansı (MHz)
#define CPU_FREQ_LOW 160        // Düşük güç modu frekansı (MHz)
#define VOLTAGE_CHECK_PIN 34    // ADC pin for voltage monitoring (optional)
#define BROWNOUT_THRESHOLD 7    // Brownout detector threshold (0-15)
#define MINIMUM_FREE_HEAP 40000 // Minimum serbest bellek (bytes)
#define HEAP_CHECK_INTERVAL 5000 // Bellek kontrol aralığı (ms)

#endif // CONFIG_H 