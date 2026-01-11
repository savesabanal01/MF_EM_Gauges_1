#pragma once

#include <Arduino.h>

class FFGauge
{
public:
    FFGauge(uint8_t Pin1, uint8_t Pin2);
    void begin();
    void attach(uint16_t Pin3, char *init);
    void detach();
    void set(int16_t messageID, char *setPoint);
    void update();

private:
    bool    _initialised;
    uint8_t _pin1, _pin2, _pin3;
    
// Function declarations
    float scaleValue(float x, float in_min, float in_max, float out_min, float out_max);
    void  setInstrumentBrightness(float value);
    void  setFuelFlow(float value);
    void  setPowerSave(bool enabled);
    void  drawGauge();

    // Variables
    float    fuelFlow = 0; 
    float    needleRotationAngle       = 0; // angle of rotation of needle based on Fuel Flow from sim  
    float    instrumentBrightness      = 255;
    bool     powerSaveFlag             = false;
    bool     showLogo                  = true;

    int oneValue = 0;
    int tenValue = 0;
    int hundredValue = 0;
    int thousandValue = 0;
};