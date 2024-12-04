#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "config.h"

class WifiManager {
private:
    AsyncWebServer& server;
    DNSServer dnsServer;
    Preferences preferences;
    
    String ssid;
    String password;
    bool isConnected;
    
    void setupAP();
    void setupWebServer();
    bool loadCredentials();
    void saveCredentials(const String& newSSID, const String& newPass);
    bool connectToWiFi();

public:
    WifiManager(AsyncWebServer& webServer) : 
        server(webServer),
        isConnected(false) {}
    
    bool begin();
    void loop();
    void resetSettings();
    bool isWiFiConnected() const { return isConnected; }
    String getIP() const;
    int getRSSI() const;
};

#endif // WIFI_MANAGER_H 