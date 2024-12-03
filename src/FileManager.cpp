#include "FileManager.h"

bool FileManager::begin() {
    Serial.println("\n=== Initializing SD Card ===");
    
    // SPI pinlerini ayarla
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    
    // SD kartı başlat
    if (!SD.begin(SD_CS)) {
        Serial.println("❌ SD Card Mount Failed");
        sdCardMounted = false;
        return false;
    }
    
    sdCardMounted = true;
    Serial.println("✅ SD Card mounted successfully");
    
    // SD kart bilgilerini kontrol et
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("❌ No SD card attached");
        return false;
    }
    
    // Kart tipini yazdır
    Serial.print("SD Card Type: ");
    switch(cardType) {
        case CARD_MMC: Serial.println("MMC"); break;
        case CARD_SD: Serial.println("SDSC"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default: Serial.println("UNKNOWN"); break;
    }
    
    // Kart boyutunu yazdır
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
    // Müzik dosyalarını yenile
    refreshMusicFiles();
    
    return true;
}

void FileManager::end() {
    SD.end();
    sdCardMounted = false;
    musicFiles.clear();
}

bool FileManager::isMusicFile(const String& filename) {
    String lowerCase = filename;
    lowerCase.toLowerCase();
    return lowerCase.endsWith(".mp3") || 
           lowerCase.endsWith(".wav") || 
           lowerCase.endsWith(".aac");
}

void FileManager::refreshMusicFiles() {
    musicFiles.clear();
    
    // SD kart bağlantısını kontrol et
    if (!sdCardMounted || !SD.cardType()) {
        Serial.println("❌ SD card not mounted, trying to remount...");
        begin();  // SD kartı yeniden başlatmayı dene
        return;
    }
    
    File root = SD.open("/");
    if (!root) {
        Serial.println("Failed to open root directory");
        return;
    }
    
    File entry;
    while (entry = root.openNextFile()) {
        if (!entry.isDirectory() && isMusicFile(entry.name())) {
            musicFiles.push_back(entry.name());
            Serial.printf("Found music file: %s\n", entry.name());
        }
        entry.close();
    }
    root.close();
    
    Serial.printf("Total music files found: %d\n", musicFiles.size());
}

std::vector<String> FileManager::getMusicFiles() {
    std::vector<String> files;
    File root = SD.open("/");
    
    while (File file = root.openNextFile()) {
        String filename = file.name();
        
        // MP3 dosyası mı kontrol et
        if (filename.endsWith(".mp3") || filename.endsWith(".MP3")) {
            // Dosya adı geçerli mi kontrol et
            if (isValidFilename(filename)) {
                Serial.printf("Found music file: %s\n", filename.c_str());
                files.push_back("/" + filename);
            } else {
                Serial.printf("⚠️ Skipping invalid filename: %s\n", filename.c_str());
            }
        }
        file.close();
    }
    
    root.close();
    Serial.printf("Total music files found: %d\n", files.size());
    return files;
}

bool FileManager::exists(const char* path) {
    return SD.exists(path);
}

File FileManager::openFile(const char* path, const char* mode) {
    return SD.open(path, mode);
}

bool FileManager::deleteFile(const char* path) {
    return SD.remove(path);
}

bool FileManager::savePlaylist(const char* filename, const std::vector<String>& playlist) {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        return false;
    }
    
    for (const String& track : playlist) {
        file.println(track);
    }
    
    file.close();
    return true;
}

bool FileManager::loadPlaylist(const char* filename, std::vector<String>& playlist) {
    File file = SD.open(filename);
    if (!file) {
        return false;
    }
    
    playlist.clear();
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            playlist.push_back(line);
        }
    }
    
    file.close();
    return true;
}

String FileManager::getSizeString(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String FileManager::getFileInfo(const char* path) {
    File file = SD.open(path);
    if (!file) {
        return "File not found";
    }
    
    String info = "Name: " + String(file.name()) + "\n";
    info += "Size: " + getSizeString(file.size()) + "\n";
    
    file.close();
    return info;
}

void FileManager::printDirectory(File dir, int numTabs) {
    while (File entry = dir.openNextFile()) {
        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }
        
        Serial.print(entry.name());
        if (entry.isDirectory()) {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        } else {
            Serial.print("\t\t");
            Serial.print(entry.size(), DEC);
            if (isMusicFile(entry.name())) {
                Serial.print("\t[MUSIC]");
            }
            Serial.println(" bytes");
        }
        entry.close();
    }
}

void FileManager::listSPIFFSContents() {
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ SPIFFS Mount Failed");
        return;
    }

    Serial.println("\n=== SPIFFS Contents ===");
    Serial.printf("Total space: %u bytes\n", SPIFFS.totalBytes());
    Serial.printf("Used space: %u bytes\n", SPIFFS.usedBytes());
    Serial.println("Files:");
    
    File root = SPIFFS.open("/");
    if (!root) {
        Serial.println("❌ Failed to open root directory");
        return;
    }
    
    File file = root.openNextFile();
    if (!file) {
        Serial.println("No files found in SPIFFS!");
    }
    
    while(file) {
        String fileName = file.name();
        size_t fileSize = file.size();
        Serial.printf("📄 %-30s\t%s\n", fileName.c_str(), getSizeString(fileSize).c_str());
        
        // Dosya içeriğini kontrol et
        if (fileName == "/index.html") {
            Serial.println("Index.html content:");
            while(file.available()) {
                Serial.write(file.read());
            }
            Serial.println("\n---End of index.html---");
        }
        
        file = root.openNextFile();
    }
    
    root.close();
    Serial.println("=====================\n");
}

bool FileManager::existsInSPIFFS(const char* path) {
    return SPIFFS.exists(path);
}

bool FileManager::isValidFilename(const String& filename) {
    // Başındaki / karakterini kontrol et ve atla
    int startIndex = filename.startsWith("/") ? 1 : 0;
    
    // Sadece alfanumerik karakterler, nokta, tire, alt çizgi ve boşluğa izin ver
    for (size_t i = startIndex; i < filename.length(); i++) {
        char c = filename.charAt(i);
        if (!isalnum(c) && c != '.' && c != '-' && c != '_' && c != ' ') {
            Serial.printf("Invalid character in filename: '%c' (0x%02x)\n", c, c);
            return false;
        }
    }
    
    // MP3 uzantısını kontrol et
    if (!filename.endsWith(".mp3") && !filename.endsWith(".MP3")) {
        Serial.println("Invalid file extension");
        return false;
    }
    
    return true;
} 