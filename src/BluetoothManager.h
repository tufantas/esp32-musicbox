#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "AudioManager.h"
#include "TimeManager.h"
#include "config.h"

// BLE Servis ve Karakteristik UUID'leri
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define PLAYBACK_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define VOLUME_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STATUS_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define TIMER_CHAR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26ab"

class BluetoothManager : public BLEServerCallbacks, public BLECharacteristicCallbacks {
private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pPlaybackCharacteristic;
    BLECharacteristic* pVolumeCharacteristic;
    BLECharacteristic* pStatusCharacteristic;
    BLECharacteristic* pTimerCharacteristic;
    
    AudioManager& audioManager;
    TimeManager& timeManager;
    
    bool deviceConnected;
    bool oldDeviceConnected;
    
    // BLE callbacks
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    void onWrite(BLECharacteristic* pCharacteristic) override;
    
    // Komut işleme
    void handlePlaybackCommand(const String& command);
    void handleVolumeCommand(const String& command);
    void handleTimerCommand(const String& command);
    
    // Status JSON oluşturma
    String createStatusJson();

public:
    BluetoothManager(AudioManager& audio, TimeManager& time) : 
        audioManager(audio),
        timeManager(time),
        deviceConnected(false),
        oldDeviceConnected(false) {}
    
    // Temel işlevler
    bool begin();
    void loop();
    
    // Status güncelleme
    void updateStatus();
    
    // Durum kontrolü
    bool isConnected() const { return deviceConnected; }
};

#endif // BLUETOOTH_MANAGER_H 