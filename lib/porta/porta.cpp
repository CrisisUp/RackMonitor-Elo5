#include "porta.h"

void Porta::begin()
{
    pinMode(Pins::SWITCH, INPUT_PULLDOWN);
    _estadoFiltrado = false;
    _brutoAnterior = false;
    _tempoUltimaMudanca = 0;
}

void Porta::update()
{
    bool bruto = (digitalRead(Pins::SWITCH) == HIGH);
    if (bruto != _brutoAnterior)
    {
        _tempoUltimaMudanca = millis();
        _brutoAnterior = bruto;
    }
    if ((millis() - _tempoUltimaMudanca) >= DEBOUNCE_PORTA_MS &&
        bruto != _estadoFiltrado)
    {
        _estadoFiltrado = bruto;
    }
}