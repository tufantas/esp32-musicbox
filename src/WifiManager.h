#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
<<<<<<< HEAD
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
=======
#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
>>>>>>> stable-power-audio
#include "config.h"

class WifiManager {
private:
    AsyncWebServer& server;
    DNSServer dnsServer;
    Preferences preferences;
<<<<<<< HEAD
    bool isConnected;
    String ssid;
    String password;
    
    void setupAP();
    void setupWebServer();
    bool connectToWiFi();
    void saveCredentials(const String& ssid, const String& password);
    bool loadCredentials();
=======
    
    String ssid;
    String password;
    bool isConnected;
    
    void setupAP();
    void setupWebServer();
    bool loadCredentials();
    void saveCredentials(const String& newSSID, const String& newPass);
    bool connectToWiFi();
>>>>>>> stable-power-audio

public:
    WifiManager(AsyncWebServer& webServer) : 
        server(webServer),
        isConnected(false) {}
    
    bool begin();
    void loop();
<<<<<<< HEAD
    bool isWiFiConnected() const { return isConnected; }
    String getIP() const;
    int getRSSI() const;
    void resetSettings();
=======
    void resetSettings();
    bool isWiFiConnected() const { return isConnected; }
    String getIP() const;
    int getRSSI() const;
>>>>>>> stable-power-audio
};

#endif // WIFI_MANAGER_H 