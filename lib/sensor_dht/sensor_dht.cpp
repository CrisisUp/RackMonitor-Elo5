#include "sensor_dht.h"

SensorDHT::SensorDHT() : _dht(DHT_PIN, DHT_TYPE) {}

void SensorDHT::begin()
{
    _dht.begin();
    _ultimaLeitura = millis();
    Serial.println(F("[INFO] [DHT] Sensor DHT22 inicializado no GPIO 13."));
}

bool SensorDHT::atualizar()
{
    unsigned long agora = millis();

    // Respeita o intervalo mínimo de 2s do DHT22
    if (agora - _ultimaLeitura < INTERVALO_MIN_MS)
        return _valida;

    _ultimaLeitura = agora;

    float t = _dht.readTemperature();
    float u = _dht.readHumidity();

    if (isnan(t) || isnan(u))
    {
        // Mantém os últimos valores válidos, mas marca como inválido
        _valida = false;
        return false;
    }

    _temperatura = t;
    _umidade = u;
    _valida = true;
    return true;
}