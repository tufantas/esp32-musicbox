#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"

class WifiManager {
private:
    AsyncWebServer& server;
    DNSServer dnsServer;
    Preferences preferences;
    bool isConnected;
    String ssid;
    String password;
    
    void setupAP();
    void setupWebServer();
    bool connectToWiFi();
    void saveCredentials(const String& ssid, const String& password);
    bool loadCredentials();

public:
    WifiManager(AsyncWebServer& webServer) : 
        server(webServer),
        isConnected(false) {}
    
    bool begin();
    void loop();
    bool isWiFiConnected() const { return isConnected; }
    String getIP() const;
    int getRSSI() const;
    void resetSettings();
};

#endif // WIFI_MANAGER_H 