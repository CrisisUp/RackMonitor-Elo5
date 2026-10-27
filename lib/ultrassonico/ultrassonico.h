#pragma once
#include <Arduino.h>
#include "../../include/config.h"

class Ultrassonico
{
public:
    void begin();
    void update();
    float distancia() const { return _distancia; }

private:
    float _distancia = -1.0f;
    unsigned long _tempoInicio = 0;
    float _medirBruto() const;
};