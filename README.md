# ESP32 Music Box

## Bağlantı Şeması

### I2C Cihazları
#### MCP4725 DAC
- VCC -> ESP32 3.3V
- GND -> ESP32 GND
- SDA -> ESP32 GPIO21
- SCL -> ESP32 GPIO22

#### DS3231 RTC
- VCC -> ESP32 3.3V
- GND -> ESP32 GND
- SDA -> ESP32 GPIO21 (DAC ile paylaşımlı)
- SCL -> ESP32 GPIO22 (DAC ile paylaşımlı)

### SD Kart Modülü (SPI)
- VCC -> ESP32 3.3V
- GND -> ESP32 GND
- MISO -> ESP32 GPIO19
- MOSI -> ESP32 GPIO23
- SCK -> ESP32 GPIO18
- CS -> ESP32 GPIO5

### RGB Status LED (Ortak Katot)
- R -> ESP32 GPIO25
- G -> ESP32 GPIO26
- B -> ESP32 GPIO27
- GND -> ESP32 GND

## Pin Özeti
| Bileşen | Pin | ESP32 GPIO |
|---------|-----|------------|
| I2C SDA | SDA | GPIO21 |
| I2C SCL | SCL | GPIO22 |
| SD MISO | MISO | GPIO19 |
| SD MOSI | MOSI | GPIO23 |
| SD SCK | SCK | GPIO18 |
| SD CS | CS | GPIO5 |
| LED Red | R | GPIO25 |
| LED Green | G | GPIO26 |
| LED Blue | B | GPIO27 |

## Özellikler
- MP3/WAV/AAC dosya çalma desteği
- Web arayüzü ile kontrol
- MQTT desteği
- Bluetooth kontrol
- RTC ile zamanlayıcı
- OTA güncelleme
- WiFi yapılandırma
- SD kart üzerinden müzik dosyası yönetimi

## Kullanım
1. Cihaz ilk açıldığında AP modunda başlar ("ESP32_MusicBox" SSID'si ile)
2. AP'ye bağlanıp WiFi ayarlarını yapın
3. Web arayüzü üzerinden müzik dosyalarını yükleyin
4. Mobil uygulama veya web arayüzü ile kontrol edin

## LED Durum Kodları
- Mavi Yanıp Sönme: Başlatılıyor
- Kırmızı: Hata Durumu
- Yeşil: Normal Çalışma
- Sarı Yanıp Sönme: WiFi Bağlanıyor

## Geliştirme
- PlatformIO ile geliştirme yapılmaktadır
- Arduino framework kullanılmaktadır
- ESP32 DevKit tabanlıdır

## Notlar
- I2C cihazları aynı bus üzerinde çalışmaktadır
- SD kart modülü 3.3V ile çalışmalıdır
- RGB LED ortak katot tipinde olmalıdır 