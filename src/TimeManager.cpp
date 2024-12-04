#include "TimeManager.h"
#include <ArduinoJson.h>

bool TimeManager::begin() {
    Serial.println("\n=== Initializing Time Manager ===");
    
    if (!rtc.begin()) {
        Serial.println("❌ Couldn't find RTC");
        return false;
    }
    
    // Derleme zamanını al
    DateTime buildTime = DateTime(F(__DATE__), F(__TIME__));
    
    // Türkiye saati için 3 saat ekle (UTC -> UTC+3)
    buildTime = buildTime + TimeSpan(0, 3, 0, 0);  // 3 saat ekle
    
    // RTC'yi ayarla
    rtc.adjust(buildTime);
    Serial.println("⚠️ RTC time adjusted to Turkey time (UTC+3)");
    
    // Mevcut zamanı göster
    DateTime current = rtc.now();
    char dateString[20];
    char timeString[20];
    
    sprintf(dateString, "%02d/%02d/%04d", current.day(), current.month(), current.year());
    sprintf(timeString, "%02d:%02d:%02d", current.hour(), current.minute(), current.second());
    
    Serial.println("✅ RTC initialized");
    Serial.printf("Current Date: %s\n", dateString);
    Serial.printf("Current Time: %s\n", timeString);
    Serial.printf("Temperature: %.2f°C\n", rtc.getTemperature());
    
    return true;
}

void TimeManager::loop() {
    if (!rtcPresent) return;
    
    // Her saniyede bir kontrol et
    uint32_t now = millis();
    if (now - lastCheck >= 1000) {
        lastCheck = now;
        checkTimers();
    }
}

void TimeManager::checkTimers() {
    DateTime now = rtc.now();
    
    for (const Timer& timer : timers) {
        if (timer.enabled && isTimeMatch(timer, now)) {
            // Timer tetiklendi
            if (timer.isPlayTimer) {
                // Play sinyali gönder
                Serial.println("Play timer triggered");
            } else {
                // Stop sinyali gönder
                Serial.println("Stop timer triggered");
            }
        }
    }
}

bool TimeManager::isTimeMatch(const Timer& timer, const DateTime& now) {
    return (timer.hour == now.hour() && timer.minute == now.minute() && now.second() == 0);
}

bool TimeManager::setDateTime(const DateTime& dt) {
    if (!rtcPresent) return false;
    rtc.adjust(dt);
    return true;
}

DateTime TimeManager::getDateTime() {
    return rtc.now();
}

float TimeManager::getTemperature() {
    return rtc.getTemperature();
}

bool TimeManager::addTimer(uint8_t hour, uint8_t minute, bool isPlayTimer) {
    // Geçerlilik kontrolü
    if (hour >= 24 || minute >= 60) return false;
    
    // Aynı zamanda başka timer var mı?
    for (const Timer& t : timers) {
        if (t.hour == hour && t.minute == minute) {
            return false;
        }
    }
    
    Timer newTimer = {
        .hour = hour,
        .minute = minute,
        .enabled = true,
        .isPlayTimer = isPlayTimer
    };
    
    timers.push_back(newTimer);
    return true;
}

bool TimeManager::removeTimer(uint8_t hour, uint8_t minute) {
    for (auto it = timers.begin(); it != timers.end(); ++it) {
        if (it->hour == hour && it->minute == minute) {
            timers.erase(it);
            return true;
        }
    }
    return false;
}

void TimeManager::clearTimers() {
    timers.clear();
}

bool TimeManager::saveTimers(const char* filename) {
    File file = SD.open(filename, FILE_WRITE);
    if (!file) return false;
    
    DynamicJsonDocument doc(1024);
    JsonArray timerArray = doc.to<JsonArray>();
    
    for (const Timer& timer : timers) {
        JsonObject timerObj = timerArray.createNestedObject();
        timerObj["hour"] = timer.hour;
        timerObj["minute"] = timer.minute;
        timerObj["enabled"] = timer.enabled;
        timerObj["isPlayTimer"] = timer.isPlayTimer;
    }
    
    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

bool TimeManager::loadTimers(const char* filename) {
    File file = SD.open(filename);
    if (!file) return false;
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    
    if (error) {
        file.close();
        return false;
    }
    
    timers.clear();
    JsonArray timerArray = doc.as<JsonArray>();
    
    for (JsonObject timerObj : timerArray) {
        Timer timer = {
            .hour = timerObj["hour"],
            .minute = timerObj["minute"],
            .enabled = timerObj["enabled"],
            .isPlayTimer = timerObj["isPlayTimer"]
        };
        timers.push_back(timer);
    }
    
    file.close();
    return true;
}

bool TimeManager::isTimerActive(uint8_t hour, uint8_t minute) {
    for (const Timer& timer : timers) {
        if (timer.hour == hour && timer.minute == minute) {
            return timer.enabled;
        }
    }
    return false;
}

void TimeManager::enableTimer(uint8_t hour, uint8_t minute, bool enable) {
    for (Timer& timer : timers) {
        if (timer.hour == hour && timer.minute == minute) {
            timer.enabled = enable;
            break;
        }
    }
} 