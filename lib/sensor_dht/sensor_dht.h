#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "../../include/config.h"

#define DHT_PIN 13
#define DHT_TYPE DHT22

class SensorDHT
{
public:
    SensorDHT(); // Construtor
    void begin();
    float temperatura();
    float umidade();

private:
    DHT _dht;
};