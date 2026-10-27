#pragma once
#include <Arduino.h>
#include "../../include/config.h"

class LDR
{
public:
    void begin();
    void update();
    bool estaAcesa() const { return _estado; }
    int leituraBruta() const { return analogRead(Pins::LDR_AO); }

private:
    bool _estado = false;
    unsigned long _tempoUltimaMudanca = 0;
    unsigned long _tempoInicio = 0;
};