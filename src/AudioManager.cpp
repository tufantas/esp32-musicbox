#include "AudioManager.h"

QueueHandle_t AudioManager::audioQueue = NULL;

bool AudioManager::begin() {
    // I2S çıkışını başlat
    out = new AudioOutputI2S(AUDIO_BUFFER_SIZE); // Buffer size'ı constructor'da belirt
    out->SetPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
    
    // I2S konfigürasyonunu ayarla
    out->SetBitsPerSample(16);
    out->SetChannels(2);
    out->SetRate(44100);
    out->SetGain(((float)currentVolume) / 100.0);
    
    // Stereo mod
    out->SetOutputModeMono(false);
    
    // Audio task'ı daha yüksek öncelikle başlat
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        8192,  // Stack size
        this,
        AUDIO_TASK_PRIORITY,     // Priority'yi config'den al
        &audioTaskHandle,
        1      // Tekrar Core 1'e alalım
    );
    
    return true;
}

void AudioManager::audioTask(void* parameter) {
    AudioManager* audio = (AudioManager*)parameter;
    String filename;
    
    while (true) {
        if (xQueueReceive(audioQueue, &filename, portMAX_DELAY)) {
            audio->processAudio();
        }
        vTaskDelay(1);  // Watchdog'u resetle
    }
}

void AudioManager::processAudio() {
    if (!currentTrack.length()) return;
    
    stopPlaying();
    
    // Debug bilgisi
    Serial.printf(" Trying to play: %s\n", currentTrack.c_str());
    
    file = new AudioFileSourceSD(currentTrack.c_str());
    if (!file) {
        Serial.println("❌ Failed to open file!");
        return;
    }
    
    id3 = new AudioFileSourceID3(file);
    if (!id3) {
        Serial.println("❌ Failed to create ID3 decoder!");
        delete file;
        return;
    }
    
    out = new AudioOutputI2S(AUDIO_BUFFER_SIZE); // Buffer size'ı constructor'da belirt
    out->SetPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
    out->SetGain(((float)currentVolume) / 100.0);
    out->SetOutputModeMono(false);
    
    mp3 = new AudioGeneratorMP3();
    if (!mp3) {
        Serial.println("❌ Failed to create MP3 decoder!");
        delete id3;
        delete file;
        delete out;
        return;
    }
    
    if (!mp3->begin(id3, out)) {
        Serial.println("❌ Failed to start MP3 playback");
        stopPlaying();
        return;
    }
    
    isPlaying = true;
    Serial.printf("▶️ Playing: %s\n", currentTrack.c_str());
    
    while (mp3->isRunning() && isPlaying) {
        if (!mp3->loop()) {
            mp3->stop();
            break;
        }
        // Her 50 iterasyonda bir watchdog'u resetle
        static int counter = 0;
        if (++counter >= 50) {
            vTaskDelay(1);
            counter = 0;
        } else {
            taskYIELD();
        }
    }
}

void AudioManager::stopPlaying() {
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
        delete out;
        out = NULL;
    }
    isPlaying = false;
}

void AudioManager::play(const String& filename) {
    if (SD.exists(filename)) {
        currentTrack = filename;
        isPlaying = true;
        xQueueSend(audioQueue, &filename, portMAX_DELAY);
    }
}

void AudioManager::pause() {
    if (isPlaying) {
        Serial.println("Pausing playback");
        isPlaying = false;
        if (out) {
            out->stop();
        }
    }
}

void AudioManager::stop() {
    Serial.println("Stopping playback");
    isPlaying = false;
    currentTrack = "";
    if (out) {
        out->stop();
    }
}

void AudioManager::setVolume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    currentVolume = volume;
    
    if (out) {
        out->SetGain(((float)volume) / 100.0);
    }
}

int AudioManager::getVolume() const {
    return currentVolume;
}

void AudioManager::loop() {
    static unsigned long lastUpdate = 0;
    const unsigned long updateInterval = 5000;  // Her 5 saniyede bir güncelle
    
    if (millis() - lastUpdate >= updateInterval) {
        lastUpdate = millis();
        if (isPlaying) {
            Serial.printf("Playing: %s (Volume: %d%%)\n", currentTrack.c_str(), currentVolume);
        }
    }
}

void AudioManager::next() {
    // Şu anki dosyanın dizinini bul
    File root = SD.open("/");
    File file = root.openNextFile();
    bool foundCurrent = false;
    String nextTrack = "";
    
    // Dosyaları tara
    while (file) {
        String fileName = file.name();
        if (fileName.endsWith(".mp3")) {  // Sadece MP3 dosyalarını kontrol et
            if (foundCurrent) {
                // Mevcut parçadan sonraki ilk MP3'ü bul
                nextTrack = fileName;
                break;
            }
            if (("/" + fileName) == currentTrack) {
                foundCurrent = true;
            }
        }
        file = root.openNextFile();
    }
    
    // Eğer sonraki parça bulunamadıysa veya son parçadaysak başa dön
    if (nextTrack.isEmpty()) {
        root.rewindDirectory();
        file = root.openNextFile();
        while (file) {
            String fileName = file.name();
            if (fileName.endsWith(".mp3")) {
                nextTrack = fileName;
                break;
            }
            file = root.openNextFile();
        }
    }
    
    // Dosyaları kapat
    file.close();
    root.close();
    
    // Yeni parçayı çal
    if (!nextTrack.isEmpty()) {
        play("/" + nextTrack);
    }
}

void AudioManager::previous() {
    // Şu anki dosyanın dizinini bul
    File root = SD.open("/");
    File file = root.openNextFile();
    String prevTrack = "";
    String currentFile = "";
    
    // Dosyaları tara
    while (file) {
        String fileName = file.name();
        if (fileName.endsWith(".mp3")) {
            if (("/" + fileName) == currentTrack) {
                // Eğer önceki parça kaydedildiyse onu kullan
                if (!prevTrack.isEmpty()) {
                    break;
                }
                // Eğer ilk parçadaysak son parçaya git
                else {
                    while (file = root.openNextFile()) {
                        String nextName = file.name();
                        if (nextName.endsWith(".mp3")) {
                            prevTrack = nextName;
                        }
                    }
                    break;
                }
            }
            prevTrack = fileName;
        }
        file = root.openNextFile();
    }
    
    // Dosyaları kapat
    file.close();
    root.close();
    
    // Önceki parçayı çal
    if (!prevTrack.isEmpty()) {
        play("/" + prevTrack);
    }
}

void AudioManager::play() {
    if (!isPlaying && currentTrack.length() > 0) {
        isPlaying = true;
        xQueueSend(audioQueue, &currentTrack, portMAX_DELAY);
    }
}

void AudioManager::printDebugInfo() {
    Serial.printf("Track: %s, Volume: %d%%, Playing: %s\n", 
        currentTrack.c_str(), 
        currentVolume,
        isPlaying ? "Yes" : "No"
    );
}

// Playlist yönetimi fonksiyonları daha sonra eklenecek 