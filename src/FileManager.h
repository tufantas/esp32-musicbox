#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <vector>
#include "config.h"

class FileManager {
private:
    bool sdCardMounted;
    std::vector<String> musicFiles;
    
    // Dosya uzantısı kontrolü
    bool isMusicFile(const String& filename);
    
    void printDirectory(File dir, int numTabs);
    
    // Yardımcı fonksiyonlar
    static String getSizeString(size_t bytes);

public:
    FileManager() : sdCardMounted(false) {}
    
    // Temel işlevler
    bool begin();
    void end();
    void refreshMusicFiles();
    
    // Dosya işlemleri
    bool exists(const char* path);
    File openFile(const char* path, const char* mode);
    bool deleteFile(const char* path);
    
    // Müzik dosyası işlemleri
    std::vector<String> getMusicFiles();
    bool saveMusicList(const char* filename);
    bool loadMusicList(const char* filename);
    
    // Playlist işlemleri
    bool savePlaylist(const char* filename, const std::vector<String>& playlist);
    bool loadPlaylist(const char* filename, std::vector<String>& playlist);
    
    // Durum kontrolü
    bool isSDCardMounted() const { return sdCardMounted; }
    size_t getMusicFileCount() const { return musicFiles.size(); }
    
    // SPIFFS işlemleri
    static void listSPIFFSContents();
    static bool existsInSPIFFS(const char* path);
    
    String getFileInfo(const char* path);
    
    bool isValidFilename(const String& filename);
};

#endif // FILE_MANAGER_H 