#pragma once
#include <Arduino.h>
#include "../../include/config.h"
#include "../../include/state.h"

class Audio
{
public:
    void begin();
    void parar();
    void forcarSilencio();
    void beepCurto(int freq = 1200, int dur = 100);
    void beepErro();
    void beepSucesso();
    void tocar(EstadoSistema estado, bool bloqueioAtivo, EstadoAmbiental ambiente);

private:
    int _prioridadeAtual = 0;
    unsigned long _tempoUltimoBip = 0;
};