#ifndef AUDIO_OUTPUT_MCP4725_H
#define AUDIO_OUTPUT_MCP4725_H

#include <Arduino.h>
#include <Adafruit_MCP4725.h>
#include "AudioOutput.h"

class AudioOutputMCP4725 : public AudioOutput 
{
private:
    Adafruit_MCP4725& dac;
    int currentVolume;
    
public:
    AudioOutputMCP4725(Adafruit_MCP4725& _dac) : 
        dac(_dac), 
        currentVolume(100) {
    }
    
    virtual ~AudioOutputMCP4725() {
        stop();
    }
    
    virtual bool begin() override {
        return true;
    }
    
    virtual bool ConsumeSample(int16_t sample[2]) override {
        // 16-bit stereo'dan 12-bit mono'ya dönüştür
        int32_t mono = (sample[0] + sample[1]) / 2;
        
        // Sesi güçlendir (2x)
        mono *= 2;
        
        // Clipping'i önle
        mono = constrain(mono, -32768, 32767);
        
        // -32768 ile 32767 arasındaki değeri 0-4095 aralığına dönüştür
        uint16_t value = map(mono, -32768, 32767, 0, 4095);
        
        // Volume uygula
        value = (value * currentVolume) / 100;
        
        // DAC'a gönder
        dac.setVoltage(value, false);
        return true;
    }
    
    virtual bool stop() override {
        dac.setVoltage(0, false);
        return true;
    }
    
    void setVolume(int volume) {
        currentVolume = constrain(volume, 0, 100);
    }
    
    // AudioOutput sınıfının diğer sanal fonksiyonlarını implement et
    virtual bool SetRate(int hz) override { return true; }
    virtual bool SetBitsPerSample(int bits) override { return true; }
    virtual bool SetChannels(int channels) override { return true; }
    virtual bool SetGain(float f) override { 
        currentVolume = constrain((int)(f * 100), 0, 100);
        return true;
    }
};

#endif // AUDIO_OUTPUT_MCP4725_H 