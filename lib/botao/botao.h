#pragma once
#include <Arduino.h>
#include "../../include/config.h"
#include "../../include/state.h"

class Botao
{
public:
    void begin();
    EventoBotao update();

private:
    bool _brutoAnterior = false;
    bool _filtrado = false;
    unsigned long _tempoUltimaMudanca = 0;
    unsigned long _tempoPressionado = 0;
    int _cliques = 0;
    unsigned long _tempoUltimoClique = 0;
    unsigned long _tempoUltimaAcao = 0;
};