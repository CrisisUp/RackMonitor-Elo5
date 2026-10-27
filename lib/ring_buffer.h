#pragma once
#include <cstdint>
#include <cstring>
#include "arduino_stubs.h" // <-- Inclui o mock (PC) ou o Arduino real (ESP32)

// =========================================================================
// RING BUFFER DE EVENTOS (v7.3)
// =========================================================================
constexpr uint16_t HIST_TAMANHO = 100;
constexpr uint8_t HIST_MSG_MAX = 64;

enum NivelLog : uint8_t
{
    NIVEL_INFO = 0,
    NIVEL_WARN = 1,
    NIVEL_ALERT = 2,
    NIVEL_CRIT = 3
};

struct EventoLog
{
    uint32_t ms;
    uint8_t nivel;
    char msg[HIST_MSG_MAX];
};

class RingBufferLog
{
public:
    void begin();
    void push(uint8_t nivel, const char *msg);
    void clear();

    uint16_t tamanho() const { return _count; }
    uint16_t head() const { return _head; }

    void dumpSerial() const;
    void dumpJSON() const;
    String toJSON() const;

    const EventoLog *getEvento(uint16_t index) const;

private:
    EventoLog _buffer[HIST_TAMANHO];
    volatile uint16_t _head = 0;
    volatile uint16_t _count = 0;
    static const char *_nomeNivel(uint8_t n);
};