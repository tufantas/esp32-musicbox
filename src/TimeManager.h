#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <vector>

struct Timer {
    DateTime dateTime;
    String action;
    bool enabled;
    String id;
};

class TimeManager {
private:
    RTC_DS3231& rtc;
    WiFiUDP ntpUDP;
    NTPClient timeClient;
    std::vector<Timer> timers;
    int utcOffset;  // Saat dilimi farkı

    // Timer yönetimi için yardımcı fonksiyonlar
    String generateTimerId();
    void checkTimers();

public:
    TimeManager(RTC_DS3231& _rtc) : 
        rtc(_rtc),
        timeClient(ntpUDP, "pool.ntp.org", 0, 60000),  // updateInterval'i 60 saniye yap
        utcOffset(3) {  // Türkiye için UTC+3
        timeClient.setUpdateInterval(60000);  // 60 saniye
    }
    
    bool begin();
    void loop();
    
    // Temel saat fonksiyonları
    DateTime getDateTime() const;
    bool setDateTime(const DateTime& dt);
    float getTemperature() const;
    
    // NTP senkronizasyonu
    bool syncFromNTP();
    bool setDateTime(const String& dateTimeStr);  // ISO format: "YYYY-MM-DDTHH:MM"
    
    // Timer yönetimi
    bool addTimer(const String& dateTimeStr, const String& action);
    bool removeTimer(const String& timerId);
    const std::vector<Timer>& getTimers() const { return timers; }
    void enableTimer(const String& timerId, bool enable);
    
    void setTimezone(int tz) {
        utcOffset = tz;
    }
    
    void adjustTimeWithTimezone(int& hour) {
        hour = (hour + utcOffset) % 24;
    }
    
    void setUtcOffset(int offset) {
        Serial.printf("Setting UTC offset to: %d\n", offset);
        utcOffset = offset;
        timeClient.end();  // NTP istemcisini yeniden başlat
        timeClient.begin();
        timeClient.setTimeOffset(offset * 3600);  // Saati saniyeye çevir
        
        // Mevcut RTC zamanını yeni offset'e göre ayarla
        DateTime now = rtc.now();
        int newHour = (now.hour() + offset) % 24;
        DateTime adjusted(now.year(), now.month(), now.day(), 
                         newHour, now.minute(), now.second());
        rtc.adjust(adjusted);
    }
    
    int getUtcOffset() const { return utcOffset; }
    
    // Sadece fonksiyon bildirimi
    bool getNTPTime(int& hour, int& minute, int& second);
};

#endif // TIME_MANAGER_H 