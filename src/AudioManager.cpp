#include "AudioManager.h"

QueueHandle_t AudioManager::audioQueue = NULL;

bool AudioManager::begin() {
    // Basit DAC kontrolü
    if (!dac.begin(MCP4725_ADDR)) {
        Serial.println("❌ MCP4725 DAC initialization failed!");
        return false;
    }
    
    // Başlangıç ses seviyesini ayarla
    dac.setVoltage(0, false);  // Başlangıçta sessiz
    setVolume(currentVolume);
    
    // Audio task'ı başlat
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        8192,
        this,
        1,
        &audioTaskHandle,
        1
    );
    
    Serial.println("✅ MCP4725 DAC initialized");
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
    
    stopPlaying();  // Önceki çalmayı durdur
    
    // Yeni çalma işlemini başlat
    file = new AudioFileSourceSD(currentTrack.c_str());
    id3 = new AudioFileSourceID3(file);
    out = new AudioOutputMCP4725(dac);
    mp3 = new AudioGeneratorMP3();
    
    // Volume'u yüksek tut
    setVolume(100);  // Maksimum volume
    
    if (!mp3->begin(id3, out)) {
        Serial.println("❌ Failed to start MP3 playback");
        stopPlaying();
        return;
    }
    
    isPlaying = true;
    Serial.printf("▶️ Playing: %s\n", currentTrack.c_str());
    Serial.println("Volume set to maximum for testing");
    
    while (mp3->isRunning() && isPlaying) {
        if (!mp3->loop()) {
            mp3->stop();
            break;
        }
        vTaskDelay(1);
    }
    
    stopPlaying();
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
    volume = constrain(volume, 0, 100);
    currentVolume = volume;
    uint16_t dacValue = volumeToDacValue(volume);
    dac.setVoltage(dacValue, false);
    Serial.printf("Volume set to: %d%%\n", volume);
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