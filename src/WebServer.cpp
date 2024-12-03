#include "WebServer.h"

bool WebServer::begin() {
    Serial.println("\n=== Initializing Web Server ===");
    
    // SPIFFS başlat
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ SPIFFS Mount Failed");
        return false;
    }
    
    // SPIFFS içeriğini kontrol et
    Serial.println("\nChecking SPIFFS files:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file) {
        Serial.printf("Found file: %s, size: %d bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }
    
    // Ana sayfa route'u
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/index.html")) {
            request->send(SPIFFS, "/index.html", "text/html");
            Serial.println("Serving index.html");
        } else {
            Serial.println("❌ index.html not found!");
            request->send(404, "text/plain", "index.html not found!");
        }
    });
    
    // Statik dosyalar için route'lar
    server.serveStatic("/css/", SPIFFS, "/css/");
    server.serveStatic("/js/", SPIFFS, "/js/");
    
    // API route'larını ayarla
    setupRoutes();
    
    // WebSocket handler'ı ekle
    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
        AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if(type == WS_EVT_DATA) {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                handleWebSocketMessage(server, client, info, data, len);
            }
        }
    });
    server.addHandler(&ws);
    
    // Sunucuyu başlat
    server.begin();
    
    // Web sunucu bilgilerini göster
    Serial.println("\n=== Web Server Started ===");
    Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("Available routes:");
    Serial.println(" - http://" + WiFi.localIP().toString() + "/");
    Serial.println(" - http://" + WiFi.localIP().toString() + "/css/style.css");
    Serial.println(" - http://" + WiFi.localIP().toString() + "/js/app.js");
    
    return true;
}

void WebServer::setupRoutes() {
    // Ana sayfa
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/index.html")) {
            request->send(SPIFFS, "/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "index.html not found!");
        }
    });
    
    // CSS dosyası için route
    server.on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/css/style.css")) {
            Serial.println("Serving style.css");
            request->send(SPIFFS, "/css/style.css", "text/css");
        } else {
            Serial.println("❌ style.css not found!");
            request->send(404, "text/plain", "File not found!");
        }
    });
    
    // JavaScript dosyası için route
    server.on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/js/app.js")) {
            Serial.println("Serving app.js");
            request->send(SPIFFS, "/js/app.js", "application/javascript");
        } else {
            Serial.println("❌ app.js not found!");
            request->send(404, "text/plain", "File not found!");
        }
    });
    
    // Catch-all handler for any other requests
    server.onNotFound([](AsyncWebServerRequest *request) {
        Serial.printf("NOT FOUND: %s\n", request->url().c_str());
        request->send(404, "text/plain", "Not found");
    });
    
    // API endpoints
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(512);
        
        // Temel durum bilgileri
        doc["wifi"] = WiFi.isConnected() ? "Connected" : "Disconnected";
        doc["mqtt"] = mqttManager.isConnectedToMqtt() ? "Connected" : "Disconnected";
        doc["volume"] = audioManager.getVolume();
        doc["track"] = audioManager.getCurrentTrack();
        doc["playing"] = audioManager.isCurrentlyPlaying();
        
        // Sıcaklık ve zaman bilgileri
        doc["temperature"] = timeManager.getTemperature();
        DateTime now = timeManager.getDateTime();
        doc["time"]["hour"] = now.hour();
        doc["time"]["minute"] = now.minute();
        
        String output;
        serializeJson(doc, output);
        response->print(output);
        request->send(response);
    });
    
    // Müzik dosyası yükleme
    server.on("/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            request->send(200);
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            handleFileUpload(request, filename, index, data, len, final);
        }
    );
    
    // Playlist yönetimi
    server.on("/api/playlist", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(4096);
        JsonArray array = doc.to<JsonArray>();
        
        auto files = fileManager.getMusicFiles();
        for (const auto& file : files) {
            String filename = file;
            if (filename.startsWith("/")) {
                filename = filename.substring(1);
            }
            array.add(filename);
        }
        
        String output;
        serializeJson(doc, output);
        response->print(output);
        request->send(response);
    });
    
    // Timer yönetimi
    server.on("/api/timers", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<1024> doc;
        JsonArray array = doc.createNestedArray("timers");
        
        auto timers = timeManager.getTimers();
        for (const auto& timer : timers) {
            JsonObject timerObj = array.createNestedObject();
            timerObj["hour"] = timer.hour;
            timerObj["minute"] = timer.minute;
            timerObj["enabled"] = timer.enabled;
            timerObj["isPlayTimer"] = timer.isPlayTimer;
        }
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    // OTA güncelleme
    server.on("/update", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            request->send(200);
            delay(1000);
            ESP.restart();
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            handleOTAUpdate(request, filename, index, data, len, final);
        }
    );
    
    // API endpoints
    server.on("/api/play", HTTP_POST, [this](AsyncWebServerRequest *request) {
        Serial.println("\n=== Play Request ===");
        
        // POST verilerini al
        if (request->_tempObject) {
            String json = String((char*)request->_tempObject);  // String kopyası oluştur
            free(request->_tempObject);  // Hemen belleği temizle
            request->_tempObject = NULL;
            
            Serial.printf("Raw body: %s\n", json.c_str());
            
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, json);
            
            if (!error && doc.containsKey("file")) {
                String file = doc["file"].as<String>();
                Serial.printf("Playing file: %s\n", file.c_str());
                
                if (file.startsWith("/")) {
                    file = file.substring(1);  // Baştaki / karakterini kaldır
                }
                
                audioManager.play("/" + file);  // Başına / ekleyerek gönder
                request->send(200);
                return;
            }
        }
        
        Serial.println("❌ Invalid play request");
        request->send(400, "text/plain", "Invalid request");
        
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Body handler
        if (!index) {
            request->_tempObject = malloc(total + 1);
            memcpy(request->_tempObject, data, len);
            ((uint8_t*)request->_tempObject)[len] = 0;
        } else {
            memcpy((uint8_t*)request->_tempObject + index, data, len);
            if (index + len == total) {
                ((uint8_t*)request->_tempObject)[total] = 0;
            }
        }
    });
    
    server.on("/api/pause", HTTP_POST, [this](AsyncWebServerRequest *request) {
        audioManager.pause();
        request->send(200);
    });
    
    server.on("/api/stop", HTTP_POST, [this](AsyncWebServerRequest *request) {
        audioManager.stop();
        request->send(200);
    });
    
    server.on("/api/prev", HTTP_POST, [this](AsyncWebServerRequest *request) {
        audioManager.previous();
        request->send(200);
    });
    
    server.on("/api/next", HTTP_POST, [this](AsyncWebServerRequest *request) {
        audioManager.next();
        request->send(200);
    });
    
    server.on("/api/volume", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("value", true)) {
            static uint32_t lastVolumeUpdate = 0;
            const uint32_t VOLUME_UPDATE_INTERVAL = 50;  // 50ms'ye düşürdük
            
            // Çok sık volume güncellemesini engelle
            uint32_t currentTime = millis();
            if (currentTime - lastVolumeUpdate < VOLUME_UPDATE_INTERVAL) {
                request->send(200);
                return;
            }
            
            int volume = request->getParam("value", true)->value().toInt();
            
            // Volume değerini sınırla
            volume = constrain(volume, 0, 100);
            
            audioManager.setVolume(volume);
            lastVolumeUpdate = currentTime;
            
            // Hızlı yanıt ver
            AsyncWebServerResponse *response = request->beginResponse(200);
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
        } else {
            request->send(400, "text/plain", "Missing volume parameter");
        }
    });
}

void WebServer::handleFileUpload(AsyncWebServerRequest *request, String filename, 
    size_t index, uint8_t *data, size_t len, bool final) {
    
    static File uploadFile;
    
    if (!index) {
        Serial.printf(" Upload Start: %s\n", filename.c_str());
        
        // Dosya adını kontrol et
        if (!filename.endsWith(".mp3")) {
            request->send(400, "text/plain", "Only MP3 files are allowed");
            return;
        }
        
        // SD kart kontrolü
        if (!SD.exists("/")) {
            request->send(500, "text/plain", "SD card not mounted");
            return;
        }
        
        uploadFile = SD.open("/" + filename, FILE_WRITE);
        if (!uploadFile) {
            request->send(500, "text/plain", "Could not create file");
            return;
        }
    }
    
    if (uploadFile) {
        if (uploadFile.write(data, len) != len) {
            uploadFile.close();
            request->send(500, "text/plain", "Write error");
            return;
        }
        
        if (final) {
            uploadFile.close();
            Serial.printf("✅ Upload Complete: %s, %u bytes\n", filename.c_str(), index + len);
            request->send(200, "text/plain", "File uploaded successfully");
        }
    }
}

void WebServer::handleOTAUpdate(AsyncWebServerRequest *request, const String& filename, 
    size_t index, uint8_t *data, size_t len, bool final) {
    
    if (!index) {
        Serial.println("OTA Update Start");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            return;
        }
    }
    
    if (Update.write(data, len) != len) {
        Update.printError(Serial);
        return;
    }
    
    if (final) {
        if (!Update.end(true)) {
            Update.printError(Serial);
        } else {
            Serial.println("OTA Update Success");
        }
    }
}

void WebServer::handleWebSocketMessage(AsyncWebSocket *server, AsyncWebSocketClient *client, 
    AwsFrameInfo *info, uint8_t *data, size_t len) {
    
    data[len] = 0;
    String message = (char*)data;
    
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (!error) {
        String command = doc["command"];
        
        if (command == "play") {
            audioManager.play();
        } else if (command == "pause") {
            audioManager.pause();
        } else if (command == "stop") {
            audioManager.stop();
        } else if (command == "volume") {
            int volume = doc["value"];
            audioManager.setVolume(volume);
        }
        
        broadcastStatus();
    }
}

void WebServer::broadcastStatus() {
    String status = createStatusJson();
    ws.textAll(status);
}

String WebServer::createStatusJson() {
    StaticJsonDocument<512> doc;
    
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

String WebServer::getContentType(const String& filename) {
    if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".json")) return "application/json";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".mp3")) return "audio/mpeg";
    else if (filename.endsWith(".wav")) return "audio/wav";
    return "text/plain";
}

String WebServer::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

void WebServer::loop() {
    ws.cleanupClients();
} 