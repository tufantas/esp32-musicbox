#include "BluetoothManager.h"

bool BluetoothManager::begin() {
    // BLE cihazını başlat
    BLEDevice::init("ESP32 Music Box");
    
    // BLE sunucusunu oluştur
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);
    
    // BLE servisini oluştur
    pService = pServer->createService(SERVICE_UUID);
    
    // Playback karakteristiğini oluştur
    pPlaybackCharacteristic = pService->createCharacteristic(
        PLAYBACK_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pPlaybackCharacteristic->addDescriptor(new BLE2902());
    pPlaybackCharacteristic->setCallbacks(this);
    
    // Volume karakteristiğini oluştur
    pVolumeCharacteristic = pService->createCharacteristic(
        VOLUME_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pVolumeCharacteristic->addDescriptor(new BLE2902());
    pVolumeCharacteristic->setCallbacks(this);
    
    // Status karakteristiğini oluştur
    pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());
    
    // Timer karakteristiğini oluştur
    pTimerCharacteristic = pService->createCharacteristic(
        TIMER_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTimerCharacteristic->addDescriptor(new BLE2902());
    pTimerCharacteristic->setCallbacks(this);
    
    // Servisi başlat
    pService->start();
    
    // Advertising başlat
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // iPhone bağlantı sorununu çözer
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("BLE device ready");
    return true;
}

void BluetoothManager::loop() {
    // Bağlantı durumu değişikliğini kontrol et
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Bağlantı sonlanması için bekle
        pServer->startAdvertising(); // Yeniden advertising başlat
        oldDeviceConnected = deviceConnected;
    }
    
    // Yeni bağlantı
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        updateStatus(); // İlk status güncellemesi
    }
}

void BluetoothManager::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE Client connected");
}

void BluetoothManager::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE Client disconnected");
}

void BluetoothManager::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        String command(value.c_str());
        
        if (pCharacteristic->getUUID().toString() == PLAYBACK_CHAR_UUID) {
            handlePlaybackCommand(command);
        }
        else if (pCharacteristic->getUUID().toString() == VOLUME_CHAR_UUID) {
            handleVolumeCommand(command);
        }
        else if (pCharacteristic->getUUID().toString() == TIMER_CHAR_UUID) {
            handleTimerCommand(command);
        }
        
        updateStatus();
    }
}

void BluetoothManager::handlePlaybackCommand(const String& command) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, command);
    
    if (!error) {
        const char* cmd = doc["command"];
        if (strcmp(cmd, "play") == 0) {
            audioManager.play();
        }
        else if (strcmp(cmd, "pause") == 0) {
            audioManager.pause();
        }
        else if (strcmp(cmd, "stop") == 0) {
            audioManager.stop();
        }
    }
}

void BluetoothManager::handleVolumeCommand(const String& command) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, command);
    
    if (!error && doc["volume"].is<int>()) {
        int volume = doc["volume"];
        audioManager.setVolume(volume);
    }
}

void BluetoothManager::handleTimerCommand(const String& command) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, command);
    
    if (!error) {
        if (doc.containsKey("hour") && doc.containsKey("minute") && doc.containsKey("action")) {
            uint8_t hour = doc["hour"];
            uint8_t minute = doc["minute"];
            const char* action = doc["action"];
            
            bool isPlayTimer = (strcmp(action, "play") == 0);
            timeManager.addTimer(hour, minute, isPlayTimer);
        }
    }
}

void BluetoothManager::updateStatus() {
    if (!deviceConnected) return;
    
    String status = createStatusJson();
    pStatusCharacteristic->setValue(status.c_str());
    pStatusCharacteristic->notify();
}

String BluetoothManager::createStatusJson() {
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