// Driver simulation for Arduino UNO

#ifndef ARDUINO_H
#define ARDUINO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define B_TRUE 0
#define B_FALSE 1

#define HIGH 1
#define LOW 0

#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 2

#define NUM_OF_DIGITAL_PINS 13
#define NUM_OF_ANALOG_PINS 12
#define PIN_TYPE_DIGITAL 0
#define PIN_TYPE_ANALOG 1
#define PIN_TYPE_UNDEFINED 3

    typedef struct Pin
    {
        uint8_t pinNumber;
        uint8_t pinMode;
        uint8_t pinValue;
    } Pin;

    static const uint8_t A0 = 18;
    static const uint8_t A1 = 19;
    static const uint8_t A2 = 20;
    static const uint8_t A3 = 21;
    static const uint8_t A4 = 22;
    static const uint8_t A5 = 23;
    static const uint8_t A6 = 24;  // D4
    static const uint8_t A7 = 25;  // D6
    static const uint8_t A8 = 26;  // D8
    static const uint8_t A9 = 27;  // D9
    static const uint8_t A10 = 28; // D10
    static const uint8_t A11 = 29; // D12

    extern Pin digitalPins[NUM_OF_DIGITAL_PINS];
    extern Pin analogPins[NUM_OF_ANALOG_PINS];

    int analogRead(uint8_t pin);
    void analogWrite(uint8_t pin, uint8_t value);
    int digitalRead(uint8_t pin);
    void digitalWrite(uint8_t pin, uint8_t value);
    void pinMode(uint8_t pin, uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif