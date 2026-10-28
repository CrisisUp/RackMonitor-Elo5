#pragma once
#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <RTClib.h>
#include <WiFi.h>
#include <time.h>
#include "../../include/config.h"
#include "../../include/state.h"

class Relogio
{
public:
    Relogio();

    bool begin();
    bool sincronizarNTP();
    void talvezRessincronizar();
    String timestamp();
    String timestampISO();
    bool disponivel() const { return _rtcDisponivel; }

    // Novo: retorna true se houver hora válida (RTC ou NTP),
    // não apenas o RTC físico.
    bool temHoraValida();

    String fonteTempoStr() const;
    bool ajustarManual(int ano, int mes, int dia, int hora, int min, int seg);
    uint32_t agoraUnix();

private:
    ThreeWire _wire;
    RtcDS1302<ThreeWire> _rtc;
    bool _rtcDisponivel = false;
    unsigned long _ultimaSincronizacaoNTP = 0;
    DateTime _agora();
};