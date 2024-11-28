#include "WifiManager.h"

bool WifiManager::begin() {
    Serial.println("\n=== WiFi Manager Debug Start ===");
    Serial.println("Function: begin()");
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
    WiFi.softAP("ESP32_MusicBox");
    
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
    // Captive portal için gerekli yönlendirmeler
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    server.on("/connectivity-check", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
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
    
    // Bağlantı isteği
    server.on("/connect", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String newSSID, newPass;
        if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
            newSSID = request->getParam("ssid", true)->value();
            newPass = request->getParam("pass", true)->value();
            
            saveCredentials(newSSID, newPass);
            request->send(200, "text/html", 
                "<html><body><h2>Settings saved!</h2>"
                "<p>ESP32 will restart and try to connect to the network.</p>"
                "<script>setTimeout(function(){window.close()},3000);</script>"
                "</body></html>");
            
            delay(3000);
            ESP.restart();
        } else {
            request->send(400, "text/plain", "Missing parameters");
        }
    });
    
    // 404 handler
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    server.begin();
    Serial.println("Web Server Started");
}

bool WifiManager::connectToWiFi() {
    if (ssid.length() == 0) {
        Serial.println("❌ No SSID configured");
        return false;
    }
    
    Serial.println("\n=== WiFi Connection Debug ===");
    Serial.printf("Attempting to connect to: %s\n", ssid.c_str());
    
    // Önceki bağlantıyı temizle
    Serial.println("1. Disconnecting from any previous networks...");
    WiFi.disconnect(true);
    delay(1000);
    
    // WiFi modunu ayarla
    Serial.println("2. Setting WiFi mode to STATION...");
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // MAC adresini göster
    Serial.printf("3. MAC Address: %s\n", WiFi.macAddress().c_str());
    
    // Bağlantıyı başlat
    Serial.println("4. Starting connection...");
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.println("5. Waiting for connection...");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.printf("Status: %d ", WiFi.status());
        switch(WiFi.status()) {
            case WL_IDLE_STATUS: Serial.println("(IDLE)"); break;
            case WL_NO_SSID_AVAIL: Serial.println("(NO SSID AVAILABLE)"); break;
            case WL_SCAN_COMPLETED: Serial.println("(SCAN COMPLETED)"); break;
            case WL_CONNECTED: Serial.println("(CONNECTED)"); break;
            case WL_CONNECT_FAILED: Serial.println("(CONNECT FAILED)"); break;
            case WL_CONNECTION_LOST: Serial.println("(CONNECTION LOST)"); break;
            case WL_DISCONNECTED: Serial.println("(DISCONNECTED)"); break;
            default: Serial.println("(UNKNOWN)"); break;
        }
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        isConnected = true;
        Serial.println("\n=== WiFi Connected Successfully ===");
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        Serial.println("==============================\n");
        return true;
    }
    
    Serial.println("\n❌ WiFi Connection Failed");
    Serial.printf("Final Status: %d\n", WiFi.status());
    Serial.println("==============================\n");
    return false;
}

void WifiManager::saveCredentials(const String& newSSID, const String& newPass) {
    preferences.putString("ssid", newSSID);
    preferences.putString("password", newPass);
}

bool WifiManager::loadCredentials() {
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    
    Serial.println("\n=== Loading Credentials ===");
    Serial.printf("Found SSID: '%s'\n", ssid.c_str());
    Serial.printf("Found password length: %d\n", password.length());
    
    return ssid.length() > 0;
}

void WifiManager::loop() {
    static unsigned long lastCheck = 0;
    const unsigned long checkInterval = 1000;
    
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
    
    if (millis() - lastCheck >= checkInterval) {
        lastCheck = millis();
        
        if (WiFi.getMode() == WIFI_AP) {
            dnsServer.processNextRequest();
        }
        
        if (WiFi.status() != WL_CONNECTED && isConnected) {
            isConnected = false;
            Serial.println("WiFi connection lost!");
            connectToWiFi();
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