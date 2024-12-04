#include "WifiManager.h"
#include <ESPmDNS.h>

bool WifiManager::begin() {
    Serial.println("\n=== WiFi Manager Debug Start ===");
    Serial.println("Function: begin()");
    
    // Memory durumunu kontrol et
    Serial.printf("Memory free: %d bytes\n", ESP.getFreeHeap());
    
    // Preferences başlat
    if (!preferences.begin("wifi-config", false)) {
        Serial.println("❌ Failed to initialize preferences");
        return false;
    }
    Serial.println("✅ Preferences initialized");
    
    // Debug: Preferences içeriğini göster
    Serial.println("\n=== Saved WiFi Credentials ===");
    Serial.printf("Saved SSID: '%s'\n", preferences.getString("ssid", "").c_str());
    Serial.printf("Password length: %d\n", preferences.getString("password", "").length());
    
    // Kayıtlı WiFi bilgilerini yükle
    if (loadCredentials()) {
        Serial.println("\n=== Loaded Credentials ===");
        Serial.printf("SSID: '%s'\n", ssid.c_str());
        Serial.printf("Password length: %d\n", password.length());
        
        if (connectToWiFi()) {
            // mDNS'i başlat
            if (!MDNS.begin("musicbox")) {
                Serial.println("❌ Error setting up MDNS responder!");
            } else {
                Serial.println("✅ mDNS responder started");
                Serial.println("Device can be reached at: http://musicbox.local");
                
                // HTTP ve OTA servislerini kaydet
                MDNS.addService("http", "tcp", 80);
                MDNS.addService("ota", "tcp", 3232);
            }
            return true;
        }
        Serial.println("❌ Connection failed with saved credentials");
    } else {
        Serial.println("❌ No saved credentials found");
    }
    
    Serial.println("\n=== Starting AP Mode ===");
    // Bağlantı başarısız, AP moduna geç
    setupAP();
    setupWebServer();
    
    return true;
}

void WifiManager::setupAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    WiFi.softAP(WIFI_AP_SSID);
    
    Serial.println("AP Mode Started");
    Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    
    // DNS sunucusunu başlat
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    if (dnsServer.start(53, "*", WiFi.softAPIP())) {
        Serial.println("DNS Server started");
    } else {
        Serial.println("DNS Server failed to start");
    }
}

void WifiManager::setupWebServer() {
    // WiFi ağlarını tara
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "[";
        int n = WiFi.scanComplete();
        if (n == -2) {
            WiFi.scanNetworks(true);
        } else if (n) {
            for (int i = 0; i < n; ++i) {
                if (i) json += ",";
                json += "{";
                json += "\"rssi\":" + String(WiFi.RSSI(i));
                json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
                json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
                json += ",\"channel\":" + String(WiFi.channel(i));
                json += ",\"secure\":" + String(WiFi.encryptionType(i));
                json += "}";
            }
            WiFi.scanDelete();
            if (WiFi.scanComplete() == -2) {
                WiFi.scanNetworks(true);
            }
        }
        json += "]";
        request->send(200, "application/json", json);
    });
    
    // Ana sayfa
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = "<html><head>";
        html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
        html += "<style>";
        html += "body{font-family:Arial;margin:20px;text-align:center;background:#f0f0f0}";
        html += ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}";
        html += ".network{padding:10px;margin:5px 0;background:#f8f8f8;cursor:pointer;border-radius:5px}";
        html += ".network:hover{background:#e8e8e8}";
        html += "input{margin:10px;padding:10px;width:80%;border:1px solid #ddd;border-radius:4px}";
        html += "button{margin:10px;padding:10px;width:80%;background:#4CAF50;color:white;border:none;border-radius:4px;cursor:pointer}";
        html += "</style>";
        html += "<script>";
        html += "function scanNetworks(){";
        html += "fetch('/scan').then(r=>r.json()).then(networks=>{";
        html += "const list=document.getElementById('networks');";
        html += "list.innerHTML='';";
        html += "networks.sort((a,b)=>b.rssi-a.rssi).forEach(n=>{";
        html += "const div=document.createElement('div');";
        html += "div.className='network';";
        html += "div.onclick=()=>selectNetwork(n.ssid);";
        html += "div.innerHTML=`${n.ssid} (${n.rssi}dBm)`;";
        html += "list.appendChild(div);";
        html += "});});};";
        html += "function selectNetwork(ssid){document.getElementById('ssid').value=ssid;}";
        html += "setInterval(scanNetworks,5000);";
        html += "</script>";
        html += "</head><body><div class='container'>";
        html += "<h1>ESP32 Music Box</h1>";
        html += "<h2>WiFi Setup</h2>";
        html += "<div id='networks'></div>";
        html += "<form method='POST' action='/connect'>";
        html += "<input type='text' id='ssid' name='ssid' placeholder='WiFi Name' required><br>";
        html += "<input type='password' name='pass' placeholder='Password' required><br>";
        html += "<button type='submit'>Connect</button>";
        html += "</form>";
        html += "<button onclick='scanNetworks()'>Scan Networks</button>";
        html += "</div></body></html>";
        request->send(200, "text/html", html);
    });
    
    // Bağlantı isteği için endpoint
    server.on("/connect", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
            String newSSID = request->getParam("ssid", true)->value();
            String newPass = request->getParam("pass", true)->value();
            
            Serial.println("\n=== New WiFi Credentials ===");
            Serial.printf("SSID: %s\n", newSSID.c_str());
            Serial.printf("Password length: %d\n", newPass.length());
            
            // Kimlik bilgilerini kaydet
            saveCredentials(newSSID, newPass);
            
            // Başarılı yanıt gönder
            String html = "<html><head>";
            html += "<meta charset='UTF-8'>";
            html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
            html += "<style>";
            html += "body{font-family:Arial;margin:20px;text-align:center;background:#f0f0f0}";
            html += ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}";
            html += "</style>";
            html += "</head><body><div class='container'>";
            html += "<h2>✅ WiFi Ayarları Kaydedildi!</h2>";
            html += "<p>ESP32 yeniden başlatılıyor ve ağa bağlanmaya çalışacak.</p>";
            html += "<p>Bu pencereyi kapatabilirsiniz.</p>";
            html += "<script>setTimeout(function(){window.close()},3000);</script>";
            html += "</div></body></html>";
            
            request->send(200, "text/html", html);
            
            // 3 saniye bekle ve yeniden başlat
            delay(3000);
            ESP.restart();
        } else {
            request->send(400, "text/plain", "SSID veya şifre eksik!");
        }
    });
    
    server.begin();
}

bool WifiManager::loadCredentials() {
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    
    Serial.println("\n=== Loading Credentials ===");
    Serial.printf("Found SSID: '%s'\n", ssid.c_str());
    Serial.printf("Found password length: %d\n", password.length());
    
    return ssid.length() > 0;
}

void WifiManager::saveCredentials(const String& newSSID, const String& newPass) {
    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPass);
    
    ssid = newSSID;
    password = newPass;
}

bool WifiManager::connectToWiFi() {
    if (ssid.length() == 0) {
        Serial.println("❌ No SSID configured");
        return false;
    }
    
    Serial.println("\n=== WiFi Connection Debug ===");
    Serial.printf("Attempting to connect to: %s\n", ssid.c_str());
    
    // WiFi'ı tamamen kapat
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(1000);
    
    // WiFi'ı yeniden başlat
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // MAC adresini göster
    Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
    
    // WiFi parametrelerini ayarla
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    // Bağlantıyı başlat
    Serial.println("Starting connection...");
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.println("Waiting for connection...");
    int attempts = 0;
    const int maxAttempts = 30;  // 30 deneme (15 saniye)
    
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        Serial.printf("Attempt %d/%d - Status: ", attempts + 1, maxAttempts);
        
        switch(WiFi.status()) {
            case WL_IDLE_STATUS: 
                Serial.println("IDLE"); 
                // IDLE durumunda biraz daha bekle
                delay(1000);
                break;
            case WL_NO_SSID_AVAIL: 
                Serial.println("NO SSID AVAILABLE"); 
                return false;  // SSID bulunamadıysa hemen çık
            case WL_SCAN_COMPLETED: 
                Serial.println("SCAN COMPLETED"); 
                break;
            case WL_CONNECTED: 
                Serial.println("CONNECTED"); 
                break;
            case WL_CONNECT_FAILED: 
                Serial.println("CONNECT FAILED"); 
                return false;  // Bağlantı başarısız olduysa hemen çık
            case WL_CONNECTION_LOST: 
                Serial.println("CONNECTION LOST"); 
                break;
            case WL_DISCONNECTED: 
                Serial.println("DISCONNECTED"); 
                break;
            default: 
                Serial.println("UNKNOWN"); 
                break;
        }
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        isConnected = true;
        String ip = WiFi.localIP().toString();
        
        Serial.println("\n=== WiFi Connected Successfully ===");
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP Address: %s\n", ip.c_str());
        Serial.println("==============================");
        Serial.println("\nWeb arayüzüne erişmek için tarayıcınızda şu adresi açın:");
        Serial.printf("http://%s\n\n", ip.c_str());
        
        return true;
    }
    
    Serial.println("\n❌ WiFi Connection Failed");
    Serial.printf("Final Status: %d\n", WiFi.status());
    Serial.println("==============================\n");
    return false;
}

void WifiManager::loop() {
    static unsigned long lastCheck = 0;
    const unsigned long checkInterval = 1000;
    
    if (millis() - lastCheck >= checkInterval) {
        lastCheck = millis();
        
        if (WiFi.getMode() == WIFI_AP) {
            dnsServer.processNextRequest();
        }
        
        // WiFi bağlantı kontrolü
        if (WiFi.status() != WL_CONNECTED && isConnected) {
            isConnected = false;
            Serial.println("WiFi connection lost!");
            connectToWiFi();
        } else if (WiFi.status() == WL_CONNECTED && !isConnected) {
            isConnected = true;
            // WiFi bağlantısı geri geldiğinde mDNS'i yeniden başlat
            if (!MDNS.begin("musicbox")) {
                Serial.println("❌ Error setting up MDNS responder!");
            } else {
                Serial.println("✅ mDNS responder restarted");
                MDNS.addService("http", "tcp", 80);
                MDNS.addService("ota", "tcp", 3232);
            }
        }
    }
    
    // Seri port komutlarını kontrol et
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "reset_wifi") {
            Serial.println("Resetting WiFi settings...");
            resetSettings();
        }
        if (cmd == "wifi_status") {
            Serial.println("\n=== WiFi Status ===");
            Serial.printf("Mode: %s\n", WiFi.getMode() == WIFI_AP ? "AP" : "STA");
            Serial.printf("Connected: %s\n", isConnected ? "Yes" : "No");
            Serial.printf("SSID: %s\n", ssid.c_str());
            Serial.printf("IP: %s\n", getIP().c_str());
            Serial.println("==================\n");
        }
    }
}

void WifiManager::resetSettings() {
    preferences.clear();
    WiFi.disconnect(true, true);
    delay(1000);
    ESP.restart();
}

String WifiManager::getIP() const {
    if (WiFi.getMode() == WIFI_STA) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

int WifiManager::getRSSI() const {
    return WiFi.RSSI();
} 