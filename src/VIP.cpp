#include "VIP.h"
// ZMPT101B voltage(VOLTAGE_PIN, 50.0);

VIP *VIP::GetInstance()
{
    static VIP instance;
    return &instance;
}

VIP::VIP()
{
    initialize();
}

void VIP::GetVoltageData(uint16_t *adc_var, float *voltage, float *voltage_ac, uint8_t loop)
{
    // ======================= Tegangan AC ======================== //

    _VRMS = 0.0f;
    for (uint8_t i = 0; i < loop; i++)
    {
        _zeroPoint = this->getZeroPoint();
        uint32_t Vsum = 0, meas_count = 0;
        int32_t Vnow = 0;
        uint32_t t_start = micros();
        while (micros() - t_start < period)
        {
            _ADC_Value = analogRead(VOLTAGE_PIN);
            Vnow = _ADC_Value - _zeroPoint;
            Vsum += (Vnow * Vnow);
            meas_count++;
        }
        _VRMS += sqrt(Vsum / meas_count) / ADC_MAX * VREF * SENSITIVITY;
    }

    *adc_var = _ADC_Value;
    *voltage = _VRMS / loop;
    *voltage_ac = AC_VOLTAGE;

    // ======================= Tegangan DC ======================== //
    // _ADC_Value = 0;
    // for (uint8_t i = 0; i < 100; i++)
    // {
    //     // _ADC_Value += random(1000, ADC_MAX) - ZERO_POIN;
    //     _ADC_Value += analogRead(VOLTAGE_PIN) - ZERO_POIN;
    // }
    // _VDC = (float)_ADC_Value / 100.0F / ADC_MAX * VREF / SENSITIVITY;
}

void VIP::GetCurrentData(uint16_t *adc_var, float *current)
{
    _ADC_Value = analogRead(CURRENT_PIN);
    // _ADC_Value = random(1000, ADC_MAX);
    if (_ADC_Value > _OLD_Current)
    {
        _OLD_Current = _ADC_Value;
    }
    else
    {

        uint32_t t_start = micros();
        if (micros() - t_start < 50)
        {
            _ADC_Value = analogRead(CURRENT_PIN);
            // _ADC_Value = random(1000, ADC_MAX);
            if (_ADC_Value < _OLD_Current)
            {
                _MAX_Current = _OLD_Current;
                _OLD_Current = 0.0f;
            }
        }
    }

    _IRMS = _MAX_Current * 3.3 * 0.707 / ADC_MAX;
    *adc_var = _ADC_Value;
    *current = _IRMS;
}

void VIP::GetPowerData(float *power, float *v_rms, float *i_rms)
{
    *power = (*i_rms) * (*i_rms) * (*v_rms);
}

int VIP::getZeroPoint()
{
    uint32_t Vsum, meas_count = 0;
    uint32_t t_start = micros();
    while (micros() - t_start < period)
    {
        Vsum += analogRead(VOLTAGE_PIN);
        meas_count++;
    }
    return Vsum / meas_count;
}

void VIP::initialize()
{
    _ADC_Value = 0;
    _VDC = 0;
    _MAX_Current = 0;
    _OLD_Current = 0;
    _IRMS = 0;
    _zeroPoint = 0;
    _freq = DEF_FREQ;
    period = 1000000 / _freq;
}