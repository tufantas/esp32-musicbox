#include "MQTTManager.h"

bool MQTTManager::begin() {
    mqttClient.onMessage([this](char* topic, char* payload,
        AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
        this->handleMessage(topic, payload, properties, len, index, total);
    });
    
    mqttClient.onConnect([this](bool sessionPresent) {
        Serial.println("Connected to MQTT");
        isConnected = true;
        reconnectAttempts = 0;
        
        // Topic'lere abone ol
        subscribe(TOPIC_COMMAND);
        subscribe(TOPIC_VOLUME);
        subscribe(TOPIC_TIMER);
        
        // Bağlantı durumunu bildir
        sendStatus(true);
    });
    
    mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason) {
        Serial.println("Disconnected from MQTT");
        isConnected = false;
    });
    
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    connect();
    
    return true;
}

void MQTTManager::loop() {
    unsigned long currentMillis = millis();
    
    // Status güncellemesi
    if (statusChanged && (currentMillis - lastStatusUpdate >= STATUS_UPDATE_INTERVAL)) {
        sendStatus();
        lastStatusUpdate = currentMillis;
        statusChanged = false;
    }
    
    // Yeniden bağlanma kontrolü
    if (!isConnected && (currentMillis - lastReconnectAttempt >= RECONNECT_DELAY)) {
        lastReconnectAttempt = currentMillis;
        
        if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            Serial.println("Attempting MQTT reconnection...");
            connect();
            reconnectAttempts++;
        }
    }
}

void MQTTManager::connect() {
    mqttClient.connect();
}

void MQTTManager::disconnect() {
    mqttClient.disconnect();
}

void MQTTManager::publish(const char* topic, const char* payload, bool retain) {
    if (isConnected) {
        mqttClient.publish(topic, 0, retain, payload);
    }
}

void MQTTManager::subscribe(const char* topic) {
    if (isConnected) {
        mqttClient.subscribe(topic, 0);
    }
}

void MQTTManager::handleMessage(char* topic, char* payload, 
    AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    
    // Payload'ı string'e çevir
    payload[len] = '\0';
    String message = String(payload);
    
    if (strcmp(topic, TOPIC_COMMAND) == 0) {
        handleCommand(message);
    }
    else if (strcmp(topic, TOPIC_VOLUME) == 0) {
        handleVolume(message);
    }
    else if (strcmp(topic, TOPIC_TIMER) == 0) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, message);
        if (!error) {
            handleTimer(doc);
        }
    }
}

void MQTTManager::handleCommand(const String& command) {
    if (command == "play") {
        audioManager.play();
    }
    else if (command == "pause") {
        audioManager.pause();
    }
    else if (command == "stop") {
        audioManager.stop();
    }
    notifyStatusChange();
}

void MQTTManager::handleVolume(const String& volume) {
    int vol = volume.toInt();
    if (vol >= 0 && vol <= 100) {
        audioManager.setVolume(vol);
        notifyStatusChange();
    }
}

void MQTTManager::handleTimer(const JsonDocument& doc) {
    if (doc.containsKey("hour") && doc.containsKey("minute") && doc.containsKey("action")) {
        uint8_t hour = doc["hour"];
        uint8_t minute = doc["minute"];
        String action = doc["action"].as<String>();
        
        bool isPlayTimer = (action == "play");
        timeManager.addTimer(hour, minute, isPlayTimer);
    }
}

void MQTTManager::sendStatus(bool force) {
    if (!isConnected) return;
    
    String status = createStatusJson();
    publish(TOPIC_STATUS, status.c_str());
}

String MQTTManager::createStatusJson() {
    StaticJsonDocument<256> doc;
    
    doc["playing"] = audioManager.isCurrentlyPlaying();
    doc["volume"] = audioManager.getVolume();
    doc["track"] = audioManager.getCurrentTrack();
    
    DateTime now = timeManager.getDateTime();
    doc["time"]["hour"] = now.hour();
    doc["time"]["minute"] = now.minute();
    doc["time"]["second"] = now.second();
    doc["temperature"] = timeManager.getTemperature();
    
    String status;
    serializeJson(doc, status);
    return status;
} 