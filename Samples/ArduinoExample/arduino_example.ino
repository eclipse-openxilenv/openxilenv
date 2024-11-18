#include "Arduino.h"

const uint8_t LED_PIN = 10;
const uint8_t ENABLE_BUTTON_PIN = 2;

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(ENABLE_BUTTON_PIN, INPUT);
}

void loop()
{
    if (digitalRead(ENABLE_BUTTON_PIN) == HIGH)
    {
        digitalWrite(LED_PIN, HIGH);
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
    }
}
