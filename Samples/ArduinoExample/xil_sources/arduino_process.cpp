
#define XILENV_INTERFACE_TYPE XILENV_DLL_INTERFACE_TYPE
#include "XilEnvExtProc.h"
#include "XilEnvExtProcMain.c"

#include "Arduino.h"

uint8_t LED_PIN = 10;
uint8_t ENABLE_BUTTON_PIN = 2;

void referenceDigitalPinValues()
{
    REF_UCHAR_VAR_AI(digitalPins[0].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                         PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[1].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[2].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[3].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[4].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[5].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[6].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[7].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[8].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[9].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[10].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[11].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[12].pinValue, "", 2, "0 0 \"LOW\"; 1 1 \"HIGH\"; 2 2 \"UNDEFINED\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
}

void referenceDigitalPinModes()
{
    REF_UCHAR_VAR_AI(digitalPins[0].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[1].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                         PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[2].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[3].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[4].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[5].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[6].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[7].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[8].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[9].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[10].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[11].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(digitalPins[12].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
}

void referenceAnalogPinValues()
{
    REF_UCHAR_VAR(analogPins[0].pinValue);
    REF_UCHAR_VAR(analogPins[1].pinValue);
    REF_UCHAR_VAR(analogPins[2].pinValue);
    REF_UCHAR_VAR(analogPins[3].pinValue);
    REF_UCHAR_VAR(analogPins[4].pinValue);
    REF_UCHAR_VAR(analogPins[5].pinValue);
    REF_UCHAR_VAR(analogPins[6].pinValue);
    REF_UCHAR_VAR(analogPins[7].pinValue);
    REF_UCHAR_VAR(analogPins[8].pinValue);
    REF_UCHAR_VAR(analogPins[9].pinValue);
    REF_UCHAR_VAR(analogPins[10].pinValue);
    REF_UCHAR_VAR(analogPins[11].pinValue);
}

void referenceAnalogPinModes()
{
    REF_UCHAR_VAR_AI(analogPins[0].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[1].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[2].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[3].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[4].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[5].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[6].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[7].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[8].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[9].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[10].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
    REF_UCHAR_VAR_AI(analogPins[11].pinMode, "", 2, "0 0 \"INPUT\"; 1 1 \"INPUT_PULLUP\"; 2 2 \"OUTPUT\"", 0, 3, WIDTH_UNDEFINED,
                     PREC_UNDEFINED, COLOR_UNDEFINED, 0, 1);
}

void reference_varis(void)
{
    REFERENCE_UCHAR_VAR(LED_PIN, "LED_Pin");
    REFERENCE_UCHAR_VAR(ENABLE_BUTTON_PIN, "EnableButton_Pin");

    referenceDigitalPinValues();
    referenceDigitalPinModes();

    referenceAnalogPinValues();
    referenceAnalogPinModes();
}

int init_test_object(void)
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(ENABLE_BUTTON_PIN, INPUT);

    return 0;
}

void cyclic_test_object(void)
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

void terminate_test_object(void)
{
}