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

// Static üye değişkenini tanımla
QueueHandle_t AudioManager::audioQueue = NULL;

bool AudioManager::begin() {
    Serial.println("\n=== Initializing Audio System ===");
    
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
        .dma_buf_count = 16,
        .dma_buf_len = 512,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    // I2S pinlerini ayarla
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    // I2S sürücüsünü başlat
    i2s_driver_uninstall(I2S_NUM_0);  // Önce kaldır
    delay(100);
    
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
        8192,
        this,
        AUDIO_TASK_PRIORITY,
        &audioTaskHandle,
        1
    );
    
    // Test sinyali gönder
    const int samples = 256;
    int16_t buffer[samples * 2];
    
    for(int i = 0; i < samples; i++) {
        float t = (float)i / 44100.0f;
        float value = sin(2.0f * PI * 1000.0f * t);
        int16_t sample = (int16_t)(value * 32767.0f);
        
        buffer[i*2] = sample;
        buffer[i*2+1] = sample;
    }
    
    for(int j = 0; j < 100; j++) {
        size_t bytes_written;
        i2s_write(I2S_NUM_0, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
    }
    
    // Sessizken çıkışı mute et
    if (out) {
        out->SetOutputModeMono(false);  // Stereo mod
        out->SetGain(0);  // Başlangıçta sessiz
    }
    
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
            delay(50);
        }
        
        currentTrack = filename;
        isPlaying = true;
        
        // Kuyruğu temizle ve yeni parçayı ekle
        xQueueReset(audioQueue);
        String* trackCopy = new String(filename);
        if (xQueueSend(audioQueue, &trackCopy, portMAX_DELAY) != pdPASS) {
            delete trackCopy;
            Serial.println("❌ Failed to send to queue");
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
    
    // Yeni kaynakları oluştur
    file = new AudioFileSourceSD(currentTrack.c_str());
    if (!file) {
        Serial.println("❌ Failed to open file!");
        return;
    }
    
    id3 = new AudioFileSourceID3(file);
    if (!id3) {
        Serial.println("❌ Failed to create ID3 decoder!");
        delete file;
        file = NULL;
        return;
    }
    
    out = new AudioOutputI2S();
    out->SetPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
    out->SetGain(((float)currentVolume) / 100.0);
    out->SetBitsPerSample(16);
    out->SetChannels(2);
    out->SetRate(44100);
    
    mp3 = new AudioGeneratorMP3();
    if (!mp3->begin(id3, out)) {
        Serial.println("❌ Failed to start MP3 playback");
        stopPlaying();
        return;
    }
    
    Serial.println("✅ MP3 playback started");
    
    // Ana çalma döngüsü
    while (mp3->isRunning() && isPlaying) {
        if (!mp3->loop()) {
            mp3->stop();
            break;
        }
        // Her 10 iterasyonda bir watchdog'u resetle
        static int counter = 0;
        if (++counter >= 10) {
            vTaskDelay(1);
            counter = 0;
        } else {
            taskYIELD();
        }
    }
}

void AudioManager::stopPlaying() {
    // Önce çalmayı durdur
    isPlaying = false;
    delay(50);  // Kısa bir bekleme ekle
    
    // Kaynakları temizle
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
    if (out) {
        out->SetGain(0);  // Önce sesi kapat
        delete out;
        out = NULL;
    }
}

void AudioManager::loop() {
    // Audio task zaten çalışıyor, burada ekstra bir şey yapmamıza gerek yok
}

void AudioManager::play() {
    if (currentTrack.length() > 0) {
        play(currentTrack);
    }
}

void AudioManager::pause() {
    if (mp3) {
        mp3->stop();
        isPlaying = false;
    }
}

void AudioManager::stop() {
    stopPlaying();
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
    
    // Volume değişimi çok küçükse işlem yapma
    if (abs(currentVolume - volume) < 2) {
        return;
    }
    
    currentVolume = volume;
    
    if (out) {
        // Volume kontrolünü doğrusal hale getir
        float gain = ((float)volume) / 100.0;  // 200.0 yerine 100.0 kullanıyoruz
        
        // Logaritmik scale uygula
        gain = pow(gain, 1.5);  // Daha doğal bir ses artışı için
        
        // Gain değerini sınırla
        if (gain > 1.0) gain = 1.0;
        if (gain < 0.0) gain = 0.0;
        
        out->SetGain(gain);
        
        // Debug
        Serial.printf("Volume: %d%%, Gain: %.2f\n", volume, gain);
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
