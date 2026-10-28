#pragma once
#include <Arduino.h>
#include "../../include/config.h"

class PIR
{
public:
    void begin();
    void update();
    bool detectaMovimento() const { return _movimento; }

private:
    bool _movimento = false;
    bool _brutoAnterior = false;
    bool _brutoFiltrado = false;
    unsigned long _tempoInicio = 0;
    unsigned long _tempoUltimaMudancaBruta = 0;
    unsigned long _tempoUltimaDeteccao = 0;

    static constexpr unsigned long DEBOUNCE_PIR_MS = 200;
    static constexpr unsigned long RETENCAO_PIR_MS = 5000;
    static constexpr unsigned long WARMUP_PIR_MS = 60000;
};