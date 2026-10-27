#include "pir.h"

void PIR::begin()
{
    pinMode(Pins::PIR, INPUT);
    _tempoInicio = millis();
}

void PIR::update()
{
    if (millis() - _tempoInicio < 1500)
    {
        _movimento = false;
        return;
    }
    _movimento = (digitalRead(Pins::PIR) == HIGH);
}