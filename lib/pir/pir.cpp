#include "pir.h"

void PIR::begin()
{
    pinMode(Pins::PIR, INPUT);
    _tempoInicio = millis();
    _tempoUltimaMudancaBruta = millis();
    _movimento = false;
    _brutoAnterior = false;
    _brutoFiltrado = false;
    _tempoUltimaDeteccao = 0;

    Serial.println(F("[PIR] Sensor iniciado. Aguardando calibracao (60s)..."));
}

void PIR::update()
{
    unsigned long agora = millis();

    // 1. Warm-up: ignora o sensor durante a calibração
    if (agora - _tempoInicio < WARMUP_PIR_MS)
    {
        _movimento = false;
        return;
    }

    // 2. Leitura bruta
    bool bruto = (digitalRead(Pins::PIR) == HIGH);

    // 3. Debounce: registra o instante da última mudança bruta
    if (bruto != _brutoAnterior)
    {
        _tempoUltimaMudancaBruta = agora;
        _brutoAnterior = bruto;
    }

    // 4. Só aceita a mudança se o sinal ficou estável por DEBOUNCE_PIR_MS
    if ((agora - _tempoUltimaMudancaBruta) >= DEBOUNCE_PIR_MS)
    {
        if (bruto && !_brutoFiltrado)
        {
            _brutoFiltrado = true;
            _movimento = true;
            _tempoUltimaDeteccao = agora;
        }
        else if (!bruto && _brutoFiltrado)
        {
            _brutoFiltrado = false;
        }
    }

    // 5. Retenção: mantém o movimento ativo por RETENCAO_PIR_MS
    if (_movimento && (agora - _tempoUltimaDeteccao) >= RETENCAO_PIR_MS)
    {
        _movimento = false;
    }
}