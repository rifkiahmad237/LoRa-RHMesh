#pragma once
#ifndef VI_H
#define VI_H

#include <Arduino.h>
#define CURRENT_PIN 32
#define VOLTAGE_PIN 33
#define CURRENT_CALLIBRATION 355.55
#define ADC_MAX 4095.0
#define VREF 3.3
#define DEF_FREQ 50.0f
#define SENSITIVITY 664.25f
#define AC_VOLTAGE 207.5f
class VIP
{
public:
    static VIP *GetInstance();
    VIP();
    void GetVoltageData(uint16_t *adc_var, float *v_rms, float *voltage_ac, uint8_t loop = 1);
    void GetCurrentData(uint16_t *adc_var, float *current);
    void GetPowerData(float *power, float *v_rms, float *i_rms);

private:
    uint16_t _ADC_Value;
    float _VCC;
    float _IRMS;
    float _VDC;
    float _VRMS;

    float _MAX_Current;
    float _OLD_Current;

    uint16_t _freq;
    uint32_t period;

    int _zeroPoint;
    int getZeroPoint();
    float setSensitivity();
    void initialize();
};

#endif