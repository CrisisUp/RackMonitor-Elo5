#include "ring_buffer.h"
#include <cstdio>

void RingBufferLog::begin()
{
    _head = 0;
    _count = 0;
}

void RingBufferLog::push(uint8_t nivel, const char *msg)
{
    noInterrupts();
    EventoLog &slot = _buffer[_head];
    slot.ms = millis();
    slot.nivel = nivel;
    strncpy(slot.msg, msg, HIST_MSG_MAX - 1);
    slot.msg[HIST_MSG_MAX - 1] = '\0';

    _head = (_head + 1) % HIST_TAMANHO;
    if (_count < HIST_TAMANHO)
        _count++;
    interrupts();
}

void RingBufferLog::clear()
{
    noInterrupts();
    _head = 0;
    _count = 0;
    interrupts();
}

const EventoLog *RingBufferLog::getEvento(uint16_t index) const
{
    if (index >= _count)
        return nullptr;
    uint16_t inicio = (_head + HIST_TAMANHO - _count) % HIST_TAMANHO;
    uint16_t idx = (inicio + index) % HIST_TAMANHO;
    return &_buffer[idx];
}

const char *RingBufferLog::_nomeNivel(uint8_t n)
{
    switch (n)
    {
    case NIVEL_INFO:
        return "INFO";
    case NIVEL_WARN:
        return "WARN";
    case NIVEL_ALERT:
        return "ALERT";
    case NIVEL_CRIT:
        return "CRIT";
    default:
        return "????";
    }
}

void RingBufferLog::dumpSerial() const
{
#ifndef NATIVE_TEST
    Serial.println(F("----------- HISTORICO DE EVENTOS -----------"));
    Serial.print(F("  Total no buffer: "));
    Serial.println(_count);
    if (_count == 0)
    {
        Serial.println(F("  (vazio)"));
        return;
    }

    uint16_t inicio = (_head + HIST_TAMANHO - _count) % HIST_TAMANHO;
    for (uint16_t i = 0; i < _count; i++)
    {
        uint16_t idx = (inicio + i) % HIST_TAMANHO;
        const EventoLog &e = _buffer[idx];
        char tempo[12];
        uint32_t s = e.ms / 1000, m = s / 60, h = m / 60;
        s %= 60;
        m %= 60;
        snprintf(tempo, sizeof(tempo), "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
        Serial.print("[");
        Serial.print(tempo);
        Serial.print("] [");
        Serial.print(_nomeNivel(e.nivel));
        Serial.print("] ");
        Serial.println(e.msg);
    }
    Serial.println(F("---------------------------------------------"));
#endif
}

void RingBufferLog::dumpJSON() const
{
#ifndef NATIVE_TEST
    Serial.println(F("{\"eventos\":["));
    if (_count == 0)
    {
        Serial.println(F("]}"));
        return;
    }
    uint16_t inicio = (_head + HIST_TAMANHO - _count) % HIST_TAMANHO;
    for (uint16_t i = 0; i < _count; i++)
    {
        uint16_t idx = (inicio + i) % HIST_TAMANHO;
        const EventoLog &e = _buffer[idx];
        Serial.print(F("  {\"ms\":"));
        Serial.print(e.ms);
        Serial.print(F(",\"nivel\":\""));
        Serial.print(_nomeNivel(e.nivel));
        Serial.print(F("\",\"msg\":\""));
        for (size_t j = 0; e.msg[j] != '\0'; j++)
        {
            char c = e.msg[j];
            if (c == '"')
                Serial.print(F("\\\""));
            else if (c == '\\')
                Serial.print(F("\\\\"));
            else if (c == '\n')
                Serial.print(F("\\n"));
            else
                Serial.print(c);
        }
        Serial.print(F("\"}"));
        if (i < _count - 1)
            Serial.print(',');
        Serial.println();
    }
    Serial.println(F("]}"));
#endif
}

String RingBufferLog::toJSON() const
{
    String json = "";
    if (_count == 0)
        return json;
    uint16_t inicio = (_head + HIST_TAMANHO - _count) % HIST_TAMANHO;
    for (uint16_t i = 0; i < _count; i++)
    {
        uint16_t idx = (inicio + i) % HIST_TAMANHO;
        const EventoLog &e = _buffer[idx];
        json += "{\"ms\":";
        json += String(e.ms);
        json += ",\"nivel\":\"";
        json += _nomeNivel(e.nivel);
        json += "\",\"msg\":\"";
        for (size_t j = 0; e.msg[j] != '\0'; j++)
        {
            char c = e.msg[j];
            if (c == '"')
                json += "\\\"";
            else if (c == '\\')
                json += "\\\\";
            else if (c == '\n')
                json += "\\n";
            else
                json += c;
        }
        json += "\"}";
        if (i < _count - 1)
            json += ",";
    }
    return json;
}