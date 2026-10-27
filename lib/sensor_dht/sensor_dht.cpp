#include "sensor_dht.h"

// O construtor inicializa o objeto DHT interno
SensorDHT::SensorDHT() : _dht(DHT_PIN, DHT_TYPE) {}

void SensorDHT::begin()
{
    _dht.begin();
    // Nota: O logSerial não está disponível aqui (dependência circular).
    // Deixamos o log para o main.cpp, ou removemos esta mensagem.
    Serial.println(F("[INFO] [DHT] Sensor DHT22 inicializado no GPIO 13."));
}

float SensorDHT::temperatura()
{
    float t = _dht.readTemperature();
    return isnan(t) ? -999.0f : t;
}

float SensorDHT::umidade()
{
    float u = _dht.readHumidity();
    return isnan(u) ? -999.0f : u;
}