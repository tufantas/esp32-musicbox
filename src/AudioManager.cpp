#include "AudioManager.h"
#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "driver/i2s.h"
#include "AudioGeneratorAAC.h"
#include "esp_heap_caps.h"

// Static üye değişkenini tanımla
QueueHandle_t AudioManager::audioQueue = NULL;

AudioManager::AudioManager(FileManager& fm) : 
    fileManager(fm),
    currentVolume(DEFAULT_VOLUME),
    isPlaying(false),
    isLooping(false),
    currentTrack(""),
    audioTaskHandle(NULL),
    file(NULL),
    id3(NULL),
    mp3(NULL),
    aac(NULL),
    out(NULL),
    trackDuration(0),
    currentPosition(0),
    lastPositionUpdate(0) {
    audioQueue = xQueueCreate(1, sizeof(String));
}

bool AudioManager::begin() {
    Serial.println("\n=== Initializing Audio System ===");
    
    // Başlangıç ses seviyesini ayarla
    currentVolume = DEFAULT_VOLUME;
    Serial.printf("Initial volume set to: %d%%\n", currentVolume);
    
    // PCM5102 pin kontrolü
    pinMode(I2S_BCLK, OUTPUT);
    pinMode(I2S_LRCK, OUTPUT);
    pinMode(I2S_DOUT, OUTPUT);
    
    // I2S konfigürasyonu
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // I2S pinlerini ayarla
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = -1
    };
    
    // I2S sürücüsünü başlat
    if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
        Serial.println("❌ Failed to install I2S driver");
        return false;
    }
    
    if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
        Serial.println("❌ Failed to set I2S pins");
        return false;
    }
    
    // Audio task'ı başlat
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        16384,
        this,
        5,
        &audioTaskHandle,
        1
    );
    
    Serial.println("✅ Audio system initialized");
    return true;
}

void AudioManager::play(const String& filename) {
    // Dosya adı kontrolü
    if (!fileManager.isValidFilename(filename)) {
        Serial.printf("❌ Invalid filename: %s\n", filename.c_str());
        return;
    }
    
    if (SD.exists(filename)) {
        // Eğer şu an çalan bir şey varsa, önce onu durdur
        if (isPlaying) {
            stopPlaying();
            delay(100);
        }
        
        currentTrack = filename;
        isPlaying = true;
        
        // Kuyruğu temizle ve yeni parçayı ekle
        xQueueReset(audioQueue);
        String* trackCopy = new String(filename);
        if (xQueueSend(audioQueue, &trackCopy, portMAX_DELAY) != pdPASS) {
            delete trackCopy;
            Serial.println("❌ Failed to send to queue");
            isPlaying = false;
            return;
        }
        
        Serial.printf("▶️ Playing: %s\n", filename.c_str());
    } else {
        Serial.printf("❌ File not found: %s\n", filename.c_str());
    }
}

void AudioManager::processAudio() {
    if (!currentTrack.length()) return;
    
    Serial.printf("\n Processing audio: %s\n", currentTrack.c_str());
    
    // Önce tüm kaynakları temizle
    if (mp3) { delete mp3; mp3 = NULL; }
    if (id3) { delete id3; id3 = NULL; }
    if (file) { delete file; file = NULL; }
    
    // Yeni kaynakları oluştur
    file = new AudioFileSourceSD(currentTrack.c_str());
    if (!file) {
        Serial.println("❌ Failed to open file!");
        stopPlaying();
        return;
    }
    
    // I2S çıkışını kontrol et
    if (!out) {
        out = new AudioOutputI2S(0, 0);
        if (!out) {
            Serial.println("❌ Failed to create I2S output!");
            delete file;
            file = NULL;
            stopPlaying();
            return;
        }
        
        // I2S ayarlarını yap
        out->SetPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
        float gain = ((float)currentVolume) / 285.0;
        gain = pow(gain, 1.8);
        if (gain > 0.35) gain = 0.35;
        if (gain < 0.0) gain = 0.0;
        out->SetGain(gain);
        out->SetBitsPerSample(16);
        out->SetChannels(2);
        out->SetRate(44100);
    }
    
    // MP3 decoder'ı oluştur
    mp3 = new AudioGeneratorMP3();
    if (!mp3) {
        Serial.println("❌ Failed to create MP3 decoder!");
        delete file;
        file = NULL;
        stopPlaying();
        return;
    }
    
    // ID3 decoder'ı oluştur
    id3 = new AudioFileSourceID3(file);
    if (!id3) {
        Serial.println("❌ Failed to create ID3 decoder!");
        delete mp3;
        mp3 = NULL;
        delete file;
        file = NULL;
        stopPlaying();
        return;
    }
    
    // Çalmayı başlat
    if (!mp3->begin(id3, out)) {
        Serial.println("❌ Failed to start playback");
        stopPlaying();
        return;
    }
    
    Serial.printf("✅ Playback started: %s\n", currentTrack.c_str());
    
    // Ana çalma döngüsü
    while (isPlaying) {
        if (mp3 && mp3->isRunning()) {
            if (!mp3->loop()) {
                Serial.println("MP3 playback ended");
                if (isLooping) {
                    // Aynı şarkıyı tekrar başlat
                    Serial.println("Restarting track (loop mode)");
                    stopPlaying();
                    delay(100);
                    play(currentTrack);
                    return;
                }
                break;
            }
            
            // Her 250ms'de bir pozisyonu güncelle
            if (millis() - lastPositionUpdate >= 250) {
                updatePosition();
                lastPositionUpdate = millis();
            }
            
            // Task'a nefes aldır
            vTaskDelay(1);
        } else {
            break;
        }
    }
    
    Serial.println("Playback complete");
    stopPlaying();
}

void AudioManager::updatePosition() {
    if (mp3 && mp3->isRunning()) {
        // Dosya pozisyonunu kullanarak yaklaşık süreyi hesapla
        if (file) {
            uint32_t currentByte = file->getPos();
            uint32_t totalBytes = file->getSize();
            if (totalBytes > 0) {
                currentPosition = (currentByte * trackDuration) / totalBytes;
                
                // Debug bilgisi
                Serial.printf("Position: %d/%d seconds (%.1f%%)\n", 
                    currentPosition, 
                    trackDuration,
                    (float)currentByte * 100 / totalBytes);
            }
        }
    }
}

void AudioManager::stopPlaying() {
    // Önce playing durumunu false yap
    isPlaying = false;
    
    // Biraz bekle
    delay(50);
    
    // Kaynakları güvenli şekilde temizle
    if (mp3) {
        mp3->stop();
        delete mp3;
        mp3 = NULL;
    }
    
    if (id3) {
        delete id3;
        id3 = NULL;
    }
    
    if (file) {
        delete file;
        file = NULL;
    }
    
    // I2S çıkışını da temizle
    if (out) {
        out->SetGain(0);
        delete out;
        out = NULL;
    }
    
    // I2S driver'ı yeniden başlat
    i2s_driver_uninstall(I2S_NUM_0);
    delay(100);
    
    // I2S konfigürasyonu
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // I2S pinlerini ayarla
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = -1
    };
    
    // I2S sürücüsünü yeniden başlat
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    
    // Diğer değişkenleri sıfırla
    trackDuration = 0;
    currentPosition = 0;
    lastPositionUpdate = 0;
    
    Serial.println("✅ Playback stopped");
}

void AudioManager::loop() {
    // Audio task zaten çalışıyor, burada ekstra bir şey yapmamıza gerek yok
}

void AudioManager::play() {
    if (currentTrack.length() > 0) {
        // Önce mevcut çalmayı durdur
        stopPlaying();
        delay(100);  // Biraz bekle
        
        // Yeni parçayı çalmaya başla
        isPlaying = true;
        
        // Audio task'a bildir
        String* trackCopy = new String(currentTrack);
        if (xQueueSend(audioQueue, &trackCopy, portMAX_DELAY) != pdPASS) {
            delete trackCopy;
            Serial.println("❌ Failed to send to queue");
            isPlaying = false;
            return;
        }
        
        Serial.printf("▶️ Resuming: %s\n", currentTrack.c_str());
    }
}

void AudioManager::pause() {
    if (mp3 && isPlaying) {
        mp3->stop();
        isPlaying = false;
        
        // Kaynakları temizle ama out'u koru
        if (mp3) { delete mp3; mp3 = NULL; }
        if (id3) { delete id3; id3 = NULL; }
        if (file) { delete file; file = NULL; }
        
        Serial.println("⏸️ Playback paused");
    }
}

void AudioManager::stop() {
    if (isPlaying) {
        Serial.println("Stopping playback...");
        stopPlaying();
    }
}

void AudioManager::next() {
    // Bir sonraki parçayı çal
    auto files = fileManager.getMusicFiles();
    if (files.size() == 0) return;

    // Mevcut parçanın indexini bul
    int currentIndex = -1;
    for (size_t i = 0; i < files.size(); i++) {
        if (files[i] == currentTrack) {
            currentIndex = i;
            break;
        }
    }

    // Bir sonraki parçaya geç
    int nextIndex = (currentIndex + 1) % files.size();
    play(files[nextIndex]);
}

void AudioManager::previous() {
    // Önceki parçayı çal
    auto files = fileManager.getMusicFiles();
    if (files.size() == 0) return;

    // Mevcut parçanın indexini bul
    int currentIndex = -1;
    for (size_t i = 0; i < files.size(); i++) {
        if (files[i] == currentTrack) {
            currentIndex = i;
            break;
        }
    }

    // Önceki parçaya geç
    int prevIndex = (currentIndex - 1 + files.size()) % files.size();
    play(files[prevIndex]);
}

void AudioManager::setVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    currentVolume = volume;
    
    if (out) {
        // Volume kontrolünü doğrusal hale getir
        float gain = ((float)volume) / 285.0;
        
        // Logaritmik scale uygula
        gain = pow(gain, 1.8);
        
        // Gain değerini sınırla
        if (gain > 0.35) gain = 0.35;
        if (gain < 0.0) gain = 0.0;
        
        out->SetGain(gain);
        
        Serial.printf("Volume: %d%%, Gain: %.3f\n", volume, gain);
    }
}

int AudioManager::getVolume() const {
    return currentVolume;
}

void AudioManager::printDebugInfo() {
    Serial.println("\n=== Audio Debug Info ===");
    Serial.printf("Current Track: %s\n", currentTrack.c_str());
    Serial.printf("Is Playing: %s\n", isPlaying ? "Yes" : "No");
    Serial.printf("Volume: %d%%\n", currentVolume);
    Serial.println("=====================\n");
}

// Static audio task
void AudioManager::audioTask(void* parameter) {
    AudioManager* audio = (AudioManager*)parameter;
    String* trackPtr;
    
    while (true) {
        if (xQueueReceive(audioQueue, &trackPtr, portMAX_DELAY)) {
            if (trackPtr) {
                audio->currentTrack = *trackPtr;  // String'i kopyala
                delete trackPtr;  // Belleği temizle
                audio->processAudio();
            }
        }
        vTaskDelay(1);
    }
}

void AudioManager::setLooping(bool enabled) {
    isLooping = enabled;
    Serial.printf("Loop mode: %s\n", enabled ? "ON" : "OFF");
}
