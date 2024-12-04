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
#define WIFI_CONNECT_TIMEOUT 30000    // 30 saniye
#define WIFI_CONNECT_DELAY 500        // 500ms

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
#ifdef DEBUG_MODE
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_STATUS(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_STATUS(x) Serial.println(x)
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
#define AUDIO_TASK_PRIORITY 3

// I2S Configuration
#define I2S_PORT I2S_NUM_0
#define I2S_DMA_BUFFER_COUNT 8
#define I2S_DMA_BUFFER_LEN 1024

#endif // CONFIG_H 