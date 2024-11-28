#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "AudioManager.h"
#include "TimeManager.h"

class MQTTManager {
private:
    AsyncMqttClient mqttClient;
    AudioManager& audioManager;
    TimeManager& timeManager;
    
    bool isConnected;
    uint8_t reconnectAttempts;
    unsigned long lastReconnectAttempt;
    unsigned long lastStatusUpdate;
    bool statusChanged;
    
    // MQTT mesaj işleme
    void handleMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    void handleCommand(const String& command);
    void handleVolume(const String& volume);
    void handleTimer(const JsonDocument& doc);
    
    // Status JSON oluşturma
    String createStatusJson();

public:
    MQTTManager(AudioManager& audio, TimeManager& time) : 
        audioManager(audio),
        timeManager(time),
        isConnected(false),
        reconnectAttempts(0),
        lastReconnectAttempt(0),
        lastStatusUpdate(0),
        statusChanged(false) {}
    
    // Temel işlevler
    bool begin();
    void loop();
    
    // Bağlantı yönetimi
    void connect();
    void disconnect();
    bool isConnectedToMqtt() const { return isConnected; }
    
    // MQTT işlemleri
    void publish(const char* topic, const char* payload, bool retain = false);
    void subscribe(const char* topic);
    
    // Status yönetimi
    void sendStatus(bool force = false);
    void notifyStatusChange() { statusChanged = true; }
};

#endif // MQTT_MANAGER_H 