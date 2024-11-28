#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "AudioManager.h"
#include "FileManager.h"
#include "TimeManager.h"
#include "MQTTManager.h"

class WebServer {
private:
    AsyncWebServer& server;
    AsyncWebSocket ws;
    AudioManager& audioManager;
    FileManager& fileManager;
    TimeManager& timeManager;
    MQTTManager& mqttManager;
    
    void setupRoutes();
    void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleOTAUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
    void handleWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsFrameInfo *info, uint8_t *data, size_t len);
    String createStatusJson();
    void broadcastStatus();
    static String getContentType(const String& filename);
    static String formatBytes(size_t bytes);

public:
    WebServer(AsyncWebServer& webServer, AudioManager& audio, FileManager& file, TimeManager& time, MQTTManager& mqtt) : 
        server(webServer),
        ws("/ws"),
        audioManager(audio),
        fileManager(file),
        timeManager(time),
        mqttManager(mqtt) {}
    
    bool begin();
    void loop();
};

#endif // WEB_SERVER_H 