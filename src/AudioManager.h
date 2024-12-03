#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "FileManager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

class AudioManager {
private:
    FileManager& fileManager;
    int currentVolume;
    bool isPlaying;
    String currentTrack;
    TaskHandle_t audioTaskHandle;
    static QueueHandle_t audioQueue;
    
    // Audio components
    AudioFileSourceSD *file;
    AudioFileSourceID3 *id3;
    AudioGeneratorMP3 *mp3;
    AudioOutputI2S *out;
    
    // Audio task
    static void audioTask(void* parameter);
    void processAudio();
    void stopPlaying();
    
    // Ses ayarları
    static const uint16_t MAX_DAC_VALUE = 4095;
    uint16_t volumeToDacValue(int volume) {
        return (uint16_t)((volume * MAX_DAC_VALUE) / 100);
    }

public:
    AudioManager(FileManager& fm) : 
        fileManager(fm),
        currentVolume(DEFAULT_VOLUME),
        isPlaying(false),
        currentTrack(""),
        audioTaskHandle(NULL),
        file(NULL),
        id3(NULL),
        mp3(NULL),
        out(NULL) {
        audioQueue = xQueueCreate(1, sizeof(String));
    }
    
    ~AudioManager() {
        stopPlaying();
        if (audioTaskHandle != NULL) {
            vTaskDelete(audioTaskHandle);
        }
        vQueueDelete(audioQueue);
    }
    
    // Temel kontroller
    bool begin();
    void play();
    void play(const String& filename);
    void pause();
    void stop();
    void next();
    void previous();
    
    // Ses kontrolü
    void setVolume(int volume);
    int getVolume() const;
    
    // Durum kontrolü
    bool isCurrentlyPlaying() const { return isPlaying; }
    String getCurrentTrack() const { return currentTrack; }
    
    // Playlist yönetimi
    bool loadPlaylist(const String& filename);
    bool savePlaylist(const String& filename);
    void addToPlaylist(const String& track);
    void removeFromPlaylist(const String& track);
    void clearPlaylist();
    
    // Zamanlayıcı işlevleri
    void startFadeIn(int targetVolume);
    void startFadeOut();
    
    // Ana döngü işlevi
    void loop();
    
    // Mevcut kodun içine ekleyin
    void printDebugInfo();
};

#endif // AUDIO_MANAGER_H 