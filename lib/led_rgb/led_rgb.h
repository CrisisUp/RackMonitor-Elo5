#pragma once
#include <Arduino.h>
#include "../../include/config.h"
#include "../../include/state.h"

class LedRGB
{
public:
    void begin();
    void definirCor(int r, int g, int b);
    void selfTest();
    void atualizar(ModoVigilante modo, EstadoSistema estado,
                   bool bloqueioAtivo, unsigned long &tempoUltimoPisca,
                   bool &estadoPisca);
};