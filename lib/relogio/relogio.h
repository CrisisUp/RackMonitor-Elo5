#pragma once
#include <Arduino.h>
#include <RTClib.h>
#include <WiFi.h>
#include <time.h>
#include "../../include/config.h"
#include "../../include/state.h"

class Relogio
{
public:
    bool begin();
    bool sincronizarNTP();
    void talvezRessincronizar();
    String timestamp();
    String timestampISO();
    bool disponivel() const { return _rtcDisponivel; }
    RTC_DS3231 &driver() { return _rtc; }
    String fonteTempoStr() const;
    bool ajustarManual(int ano, int mes, int dia, int hora, int min, int seg);

private:
    RTC_DS3231 _rtc;
    bool _rtcDisponivel = false;
    unsigned long _ultimaSincronizacaoNTP = 0;
    DateTime _agora();
};