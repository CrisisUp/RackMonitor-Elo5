#include "botao_panico.h"

volatile bool BotaoPanico::_flagISR = false;

void __attribute__((noinline)) IRAM_ATTR BotaoPanico::_isr()
{
    _flagISR = true;
}

void BotaoPanico::begin()
{
    pinMode(Pins::BOTAO_PANICO, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(Pins::BOTAO_PANICO), _isr, FALLING);
    _flagISR = false;
    _tempoUltimoDisparo = 0;
}

bool BotaoPanico::consumir()
{
    if (millis() - _tempoUltimoDisparo < COOLDOWN_MS)
    {
        _flagISR = false;
        return false;
    }
    if (_flagISR)
    {
        _flagISR = false;
        _tempoUltimoDisparo = millis();
        return true;
    }
    return false;
}