#include "porta.h"

void Porta::begin()
{
    // Sensor físico (reed switch) ligado entre o pino e GND.
    // Pull-up interno: pino em HIGH quando o circuito está aberto
    // (porta aberta) e LOW quando o ímã fecha o reed switch (porta fechada).
    pinMode(Pins::SWITCH, INPUT_PULLUP);

    _estadoFiltrado = false; // assume porta fechada no boot
    _brutoAnterior = false;
    _tempoUltimaMudanca = 0;
}

void Porta::update()
{
    // Com INPUT_PULLUP:
    //   LOW  -> reed switch fechado -> ímã perto -> porta FECHADA
    //   HIGH -> reed switch aberto -> ímã longe -> porta ABERTA
    //
    // Queremos que _estadoFiltrado = true signifique "porta aberta".
    bool bruto = (digitalRead(Pins::SWITCH) == HIGH);

    // Debounce: registra o instante da última mudança bruta
    if (bruto != _brutoAnterior)
    {
        _tempoUltimaMudanca = millis();
        _brutoAnterior = bruto;
    }

    // Só aceita a mudança se o sinal ficou estável por DEBOUNCE_PORTA_MS
    if ((millis() - _tempoUltimaMudanca) >= DEBOUNCE_PORTA_MS &&
        bruto != _estadoFiltrado)
    {
        _estadoFiltrado = bruto;
    }
}