#include "ultrassonico.h"

void Ultrassonico::begin()
{
    pinMode(Pins::TRIG, OUTPUT);
    pinMode(Pins::ECHO, INPUT);
    digitalWrite(Pins::TRIG, LOW);
    _tempoInicio = millis();
    _distancia = -1.0f;
}

void Ultrassonico::update()
{
    if (millis() - _tempoInicio < 1500)
    {
        _distancia = -1.0f;
        return;
    }
    float soma = 0.0f;
    int validas = 0;
    for (int i = 0; i < US_AMOSTRAS; i++)
    {
        float d = _medirBruto();
        if (d >= 2.0f && d < US_DIST_MAX_VALIDA)
        {
            soma += d;
            validas++;
        }
        delayMicroseconds(60);
    }
    _distancia = (validas > 0) ? (soma / validas) : -1.0f;
}

float Ultrassonico::_medirBruto() const
{
    digitalWrite(Pins::TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(Pins::TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(Pins::TRIG, LOW);
    long duracao = pulseIn(Pins::ECHO, HIGH, 25000);
    float d = duracao * 0.0343f / 2.0f;
    return (d <= 0.0f) ? -1.0f : d;
}