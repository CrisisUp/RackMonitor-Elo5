#include "ldr.h"

void LDR::begin()
{
    pinMode(Pins::LDR_AO, INPUT);
    _estado = false;
    _tempoUltimaMudanca = 0;
    _tempoInicio = millis();
}

void LDR::update()
{
    if (millis() - _tempoInicio < 1500)
    {
        _estado = false;
        return;
    }
    int leitura = analogRead(Pins::LDR_AO);
    if (!_estado && leitura < LDR_LIMIAR_APAGA)
    {
        if (millis() - _tempoUltimaMudanca >= DEBOUNCE_LDR_MS)
        {
            _estado = true;
            _tempoUltimaMudanca = millis();
        }
    }
    else if (_estado && leitura > LDR_LIMIAR_ACENDE)
    {
        if (millis() - _tempoUltimaMudanca >= DEBOUNCE_LDR_MS)
        {
            _estado = false;
            _tempoUltimaMudanca = millis();
        }
    }
}