#include "Arduino.h"

Pin digitalPins[NUM_OF_DIGITAL_PINS] = {
    {1, INPUT, LOW},
    {2, INPUT, LOW},
    {3, INPUT, LOW},
    {4, INPUT, LOW},
    {5, INPUT, LOW},
    {6, INPUT, LOW},
    {7, INPUT, LOW},
    {8, INPUT, LOW},
    {9, INPUT, LOW},
    {10, INPUT, LOW},
    {11, INPUT, LOW},
    {12, INPUT, LOW},
    {13, INPUT, LOW}};

Pin analogPins[NUM_OF_ANALOG_PINS] = {
    {0, INPUT, 0},
    {1, INPUT, 0},
    {2, INPUT, 0},
    {3, INPUT, 0},
    {4, INPUT, 0},
    {5, INPUT, 0},
    {6, INPUT, 0},
    {7, INPUT, 0},
    {8, INPUT, 0},
    {9, INPUT, 0},
    {10, INPUT, 0},
    {11, INPUT, 0}};

static uint8_t getPinType(uint8_t pin)
{
    if (pin >= 1 || pin <= NUM_OF_DIGITAL_PINS)
    {
        return PIN_TYPE_DIGITAL;
    }
    if (pin >= 18 || pin <= NUM_OF_ANALOG_PINS)
    {
        return PIN_TYPE_ANALOG;
    }

    return PIN_TYPE_UNDEFINED;
}

static uint8_t isAnalogMapKeyInvalid(uint8_t key)
{
    if (key < 18 || key > 29)
    {
        return B_TRUE;
    }

    return B_FALSE;
}

static uint8_t isDigitalMapKeyInvalid(uint8_t key)
{
    if (key < 1 || key > 13)
    {
        return B_TRUE;
    }

    return B_FALSE;
}

static uint8_t mapAnalogPin(uint8_t pinNumberToMap)
{
    if (isAnalogMapKeyInvalid(pinNumberToMap) == 1)
    {
        return 255;
    }

    return pinNumberToMap - 18;
}

static uint8_t mapDigitalPin(uint8_t pinNumberToMap)
{
    if (isDigitalMapKeyInvalid(pinNumberToMap) == B_TRUE)
    {
        return 255;
    }

    return pinNumberToMap - 1;
}

static uint8_t verifyPinMode(uint8_t pin, uint8_t pinMode, uint8_t pinType)
{
    if (analogPins[pin].pinMode == pinMode && pinType == PIN_TYPE_ANALOG)
    {
        return B_TRUE;
    }
    if (digitalPins[pin].pinMode == pinMode && pinType == PIN_TYPE_DIGITAL)
    {
        return B_TRUE;
    }

    return B_FALSE;
}

int analogRead(uint8_t pin)
{
    uint8_t pinType = getPinType(pin);

    if (pinType == PIN_TYPE_DIGITAL || pinType == PIN_TYPE_UNDEFINED)
    {
        return 0;
    }

    uint8_t mappedPin = mapAnalogPin(pin);

    if (verifyPinMode(mappedPin, OUTPUT, PIN_TYPE_ANALOG) == B_TRUE)
    {
        return 0;
    }

    return analogPins[mappedPin].pinValue;
}

void analogWrite(uint8_t pin, uint8_t value)
{
    uint8_t pinType = getPinType(pin);

    if (pinType == PIN_TYPE_DIGITAL || pinType == PIN_TYPE_UNDEFINED)
    {
        return;
    }

    uint8_t mappedPin = mapAnalogPin(pin);

    if (verifyPinMode(mappedPin, OUTPUT, PIN_TYPE_ANALOG) == B_FALSE)
    {
        return;
    }

    analogPins[mappedPin].pinValue = value;
}

void pinMode(uint8_t pin, uint8_t mode)
{

    uint8_t pinType = getPinType(pin);

    if (pinType == PIN_TYPE_UNDEFINED)
    {
        return;
    }
    if (pinType == PIN_TYPE_DIGITAL)
    {
        uint8_t mappedPin = mapDigitalPin(pin);

        if (mode == INPUT_PULLUP)
        {
            digitalPins[mappedPin].pinValue = HIGH;
        }

        digitalPins[mappedPin].pinMode = mode;
        return;
    }
    if (pinType == PIN_TYPE_ANALOG)
    {
        uint8_t mappedPin = mapAnalogPin(pin);
        analogPins[mappedPin].pinMode = mode;
        return;
    }

    return;
}

int digitalRead(uint8_t pin)
{
    uint8_t pinType = getPinType(pin);

    if (pinType == PIN_TYPE_ANALOG || pinType == PIN_TYPE_UNDEFINED)
    {
        return 255;
    }

    uint8_t mappedPin = mapDigitalPin(pin);

    if (verifyPinMode(mappedPin, OUTPUT, PIN_TYPE_DIGITAL) == B_TRUE)
    {
        return 255;
    }

    return digitalPins[mappedPin].pinValue;
}

void digitalWrite(uint8_t pin, uint8_t value)
{
    uint8_t pinType = getPinType(pin);

    if (pinType == PIN_TYPE_ANALOG || pinType == PIN_TYPE_UNDEFINED)
    {
        return;
    }

    uint8_t mappedPin = mapDigitalPin(pin);

    if (verifyPinMode(mappedPin, OUTPUT, PIN_TYPE_DIGITAL) == B_FALSE)
    {
        return;
    }

    digitalPins[mappedPin].pinValue = value;
}