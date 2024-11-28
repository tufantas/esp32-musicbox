#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>
#include <vector>
#include <SD.h>  // SD kart için
#include <FS.h>  // File sistemi için
#include "config.h"

struct Timer {
    uint8_t hour;
    uint8_t minute;
    bool enabled;
    bool isPlayTimer;  // true = play timer, false = stop timer
};

class TimeManager {
private:
    RTC_DS3231& rtc;
    std::vector<Timer> timers;
    bool rtcPresent;
    
    // Son kontrol zamanı
    uint32_t lastCheck;
    
    // Timer kontrolü
    void checkTimers();
    bool isTimeMatch(const Timer& timer, const DateTime& now);

public:
    TimeManager(RTC_DS3231& _rtc) : 
        rtc(_rtc), 
        rtcPresent(false),
        lastCheck(0) {}
    
    // Temel işlevler
    bool begin();
    void loop();
    
    // RTC işlemleri
    bool setDateTime(const DateTime& dt);
    DateTime getDateTime();
    float getTemperature();
    bool isRTCPresent() const { return rtcPresent; }
    
    // Timer işlemleri
    bool addTimer(uint8_t hour, uint8_t minute, bool isPlayTimer);
    bool removeTimer(uint8_t hour, uint8_t minute);
    void clearTimers();
    std::vector<Timer> getTimers() const { return timers; }
    bool saveTimers(const char* filename);
    bool loadTimers(const char* filename);
    
    // Timer durumu
    bool isTimerActive(uint8_t hour, uint8_t minute);
    void enableTimer(uint8_t hour, uint8_t minute, bool enable);
};

#endif // TIME_MANAGER_H 