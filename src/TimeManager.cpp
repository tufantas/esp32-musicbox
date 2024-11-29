#include "TimeManager.h"

bool TimeManager::begin() {
    timeClient.begin();
    return true;
}

void TimeManager::loop() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck >= 1000) {  // Her saniye
        lastCheck = millis();
        checkTimers();
    }
}

bool TimeManager::syncFromNTP() {
    if (!WiFi.isConnected()) {
        Serial.println("WiFi not connected for NTP sync");
        return false;
    }
    
    Serial.printf("Starting NTP sync with UTC offset: %d\n", utcOffset);
    
    // NTP istemcisini yeniden yapılandır
    timeClient.end();
    timeClient.begin();
    timeClient.setTimeOffset(utcOffset * 3600);
    
    if (timeClient.forceUpdate()) {
        time_t epochTime = timeClient.getEpochTime();
        struct tm timeinfo;
        gmtime_r(&epochTime, &timeinfo);
        
        // Yerel saat için offset uygula
        timeinfo.tm_hour = (timeinfo.tm_hour + utcOffset) % 24;
        
        DateTime localTime(
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec
        );
        
        rtc.adjust(localTime);
        
        Serial.printf("NTP sync successful! Local time: %02d:%02d:%02d UTC%+d\n",
            localTime.hour(), localTime.minute(), localTime.second(), utcOffset);
        
        return true;
    }
    
    Serial.println("NTP sync failed!");
    return false;
}

bool TimeManager::setDateTime(const String& dateTimeStr) {
    Serial.printf("Setting date/time: %s with UTC offset: %d\n", dateTimeStr.c_str(), utcOffset);
    
    int year = dateTimeStr.substring(0, 4).toInt();
    int month = dateTimeStr.substring(5, 7).toInt();
    int day = dateTimeStr.substring(8, 10).toInt();
    int hour = dateTimeStr.substring(11, 13).toInt();
    int minute = dateTimeStr.substring(14, 16).toInt();
    
    // UTC'den yerel saate çevir
    hour = (hour + utcOffset) % 24;
    
    DateTime dt(year, month, day, hour, minute, 0);
    rtc.adjust(dt);
    
    Serial.printf("DateTime set to: %04d-%02d-%02d %02d:%02d:00 UTC%+d\n",
        year, month, day, hour, minute, utcOffset);
    
    return true;
}

bool TimeManager::getNTPTime(int& hour, int& minute, int& second) {
    if (!WiFi.isConnected()) return false;
    
    if (timeClient.forceUpdate()) {
        time_t epochTime = timeClient.getEpochTime();
        struct tm timeinfo;
        gmtime_r(&epochTime, &timeinfo);
        
        // Yerel saat için offset uygula
        hour = (timeinfo.tm_hour + utcOffset) % 24;
        minute = timeinfo.tm_min;
        second = timeinfo.tm_sec;
        
        Serial.printf("NTP Time: %02d:%02d:%02d UTC%+d\n", 
            hour, minute, second, utcOffset);
        
        return true;
    }
    return false;
}

DateTime TimeManager::getDateTime() const {
    return rtc.now();
}

float TimeManager::getTemperature() const {
    return rtc.getTemperature();
}

String TimeManager::generateTimerId() {
    static int counter = 0;
    return String(millis()) + String(counter++);
}

bool TimeManager::addTimer(const String& dateTimeStr, const String& action) {
    // Format: "YYYY-MM-DDTHH:MM"
    int year = dateTimeStr.substring(0, 4).toInt();
    int month = dateTimeStr.substring(5, 7).toInt();
    int day = dateTimeStr.substring(8, 10).toInt();
    int hour = dateTimeStr.substring(11, 13).toInt();
    int minute = dateTimeStr.substring(14, 16).toInt();
    
    Timer timer;
    timer.dateTime = DateTime(year, month, day, hour, minute, 0);
    timer.action = action;
    timer.enabled = true;
    timer.id = generateTimerId();
    
    timers.push_back(timer);
    return true;
}

bool TimeManager::removeTimer(const String& timerId) {
    for (auto it = timers.begin(); it != timers.end(); ++it) {
        if (it->id == timerId) {
            timers.erase(it);
            return true;
        }
    }
    return false;
}

void TimeManager::enableTimer(const String& timerId, bool enable) {
    for (auto& timer : timers) {
        if (timer.id == timerId) {
            timer.enabled = enable;
            break;
        }
    }
}

void TimeManager::checkTimers() {
    DateTime now = rtc.now();
    
    for (auto& timer : timers) {
        if (timer.enabled && 
            timer.dateTime.year() == now.year() &&
            timer.dateTime.month() == now.month() &&
            timer.dateTime.day() == now.day() &&
            timer.dateTime.hour() == now.hour() &&
            timer.dateTime.minute() == now.minute()) {
            
            // Timer'ı devre dışı bırak
            timer.enabled = false;
            
            // Timer eylemini gerçekleştir
            // Bu kısmı AudioManager ile entegre etmek gerekecek
        }
    }
}
  