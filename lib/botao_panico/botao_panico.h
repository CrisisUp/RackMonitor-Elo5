#pragma once
#include <Arduino.h>
#include "../../include/config.h"

class BotaoPanico
{
public:
    void begin();
    bool consumir();

private:
    static volatile bool _flagISR;
    unsigned long _tempoUltimoDisparo = 0;
    static constexpr unsigned long COOLDOWN_MS = 2000UL;
    static void IRAM_ATTR _isr();
};