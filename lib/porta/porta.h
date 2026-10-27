#pragma once
#include <Arduino.h>
#include "../../include/config.h"

class Porta
{
public:
    void begin();
    void update();
    bool estaAberta() const { return _estadoFiltrado; }

private:
    bool _estadoFiltrado = false;
    bool _brutoAnterior = false;
    unsigned long _tempoUltimaMudanca = 0;
};