#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "../../include/config.h"

// Pino e tipo do DHT ficam no config.h (evita conflito de macros)
#ifndef DHT_PIN
#define DHT_PIN 13
#endif
#ifndef DHT_TYPE
#define DHT_TYPE DHT22
#endif

class SensorDHT
{
public:
    SensorDHT();
    void begin();

    // Faz uma única leitura (a cada 2s no máximo) e atualiza o cache.
    // Retorna true se a leitura foi válida.
    bool atualizar();

    // Retorna o valor em cache (ou -999 se a última leitura falhou).
    float temperatura() const { return _temperatura; }
    float umidade() const { return _umidade; }

    // Indica se a última leitura foi válida.
    bool leituraValida() const { return _valida; }

private:
    DHT _dht;
    float _temperatura = -999.0f;
    float _umidade = -999.0f;
    bool _valida = false;
    unsigned long _ultimaLeitura = 0;

    static constexpr unsigned long INTERVALO_MIN_MS = 2000; // DHT22: 2s entre leituras
};