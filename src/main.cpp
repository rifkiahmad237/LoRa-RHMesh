#define RH_TEST_NETWORK 2
#include "DataHandling.h"
#include "VIP.h"
unsigned long currentMillis;
unsigned long prevMillis1 = 0;
unsigned long prevMillis2 = 0;
power_message powerMessage;
void blinkLED_TX(const long);
void ReadVIP(const long);
void setup()
{
    DataHandling::GetInstance();
}

void loop()
{
    currentMillis = millis();
    blinkLED_TX(1000);
    ReadVIP(5000);
}

void blinkLED_TX(const long interval)
{

    if (currentMillis - prevMillis1 >= interval)
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        prevMillis1 = currentMillis;
    }
}

void ReadVIP(const long interval)
{
    uint16_t adc_value;
    float v_rms;
    float v_ac;
    float current;
    float power;
    if (currentMillis - prevMillis2 >= interval)
    {
        VIP::GetInstance()->GetVoltageData(&adc_value, &v_rms, &v_ac);
        Serial.printf("=======\nADC Voltage: %d\n", adc_value);
        Serial.printf("Sirine Voltage (VIP): %f\n", v_rms);
        Serial.printf("AC Voltage: %f \n=======\n", v_ac);

        VIP::GetInstance()->GetCurrentData(&adc_value, &current);
        Serial.printf("ADC Current: %d\n", adc_value);
        Serial.printf("Sirine Current: %f\n", current);

        VIP::GetInstance()->GetPowerData(&power, &v_rms, &current);
        Serial.printf("Sirine Power: %f\n\n", power);

        powerMessage = {v_rms, current, power};
        DataHandling::GetInstance()->sendDataToGateway((const uint8_t *)&powerMessage);
        prevMillis2 = currentMillis;
    }
}