#include "BluetoothManager.h"

bool BluetoothManager::begin() {
    Serial.println("\n=== Initializing Bluetooth ===");
    
    // BLE Device'ı başlat
    BLEDevice::init("ESP32-MusicBox");
    Serial.println("1. BLE Device initialized");
    
    // BLE Server oluştur
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);
    Serial.println("2. BLE Server created");
    
    // BLE Service oluştur
    pService = pServer->createService(SERVICE_UUID);
    
    // Karakteristikler oluştur
    pPlaybackCharacteristic = pService->createCharacteristic(
        PLAYBACK_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pPlaybackCharacteristic->setCallbacks(this);
    pPlaybackCharacteristic->addDescriptor(new BLE2902());
    
    pVolumeCharacteristic = pService->createCharacteristic(
        VOLUME_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pVolumeCharacteristic->setCallbacks(this);
    pVolumeCharacteristic->addDescriptor(new BLE2902());
    
    pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());
    
    pTimerCharacteristic = pService->createCharacteristic(
        TIMER_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTimerCharacteristic->setCallbacks(this);
    pTimerCharacteristic->addDescriptor(new BLE2902());
    
    // Servisi başlat
    pService->start();
    
    // Advertising başlat
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("5. Started advertising");
    
    return true;
}

void BluetoothManager::onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("✅ Client connected");
}

void BluetoothManager::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("❌ Client disconnected");
}

void BluetoothManager::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
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
}

void BluetoothManager::handlePlaybackCommand(const String& command) {
    if (command == "play") {
        audioManager.play();
    }
    else if (command == "pause") {
        audioManager.pause();
    }
    else if (command == "stop") {
        audioManager.stop();
    }
    else if (command == "next") {
        audioManager.next();
    }
    else if (command == "prev") {
        audioManager.previous();
    }
}

void BluetoothManager::handleVolumeCommand(const String& command) {
    int volume = command.toInt();
    audioManager.setVolume(volume);
}

void BluetoothManager::handleTimerCommand(const String& command) {
    // Timer komutlarını işle
    // JSON formatında: {"hour":12,"minute":30,"enabled":true,"isPlayTimer":true}
    DynamicJsonDocument doc(200);
    deserializeJson(doc, command);
    
    uint8_t hour = doc["hour"];
    uint8_t minute = doc["minute"];
    bool enabled = doc["enabled"];
    bool isPlayTimer = doc["isPlayTimer"];
    
    // Timer'ı ayarla
    // timeManager.setTimer(hour, minute, enabled, isPlayTimer);
}

String BluetoothManager::createStatusJson() {
    DynamicJsonDocument doc(200);
    
    doc["playing"] = audioManager.isCurrentlyPlaying();
    doc["volume"] = audioManager.getVolume();
    doc["track"] = audioManager.getCurrentTrack();
    doc["temperature"] = timeManager.getTemperature();
    
    String status;
    serializeJson(doc, status);
    return status;
}

void BluetoothManager::updateStatus() {
    if (deviceConnected) {
        String status = createStatusJson();
        pStatusCharacteristic->setValue(status.c_str());
        pStatusCharacteristic->notify();
    }
}

void BluetoothManager::loop() {
    if (deviceConnected) {
        updateStatus();
    }
    
    // Bağlantı durumu değişikliğini kontrol et
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Bağlantı sonlanması için bekle
        pServer->startAdvertising(); // Yeniden advertising başlat
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
} 