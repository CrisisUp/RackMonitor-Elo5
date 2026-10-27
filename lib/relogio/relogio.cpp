#include "relogio.h"

// Nota: logSerial ainda está no main.cpp. Para evitar dependência circular,
// usamos Serial.println diretamente aqui.
// Se quiser, podemos mover o logSerial para um header futuro.

bool Relogio::begin()
{
    if (_rtc.begin())
    {
        _rtcDisponivel = true;
        fonteTempoAtual = FONTE_RTC; // Variável global do main.cpp
        if (_rtc.lostPower())
        {
            Serial.println(F("[WARN] RTC perdeu energia. Aguardando sync NTP ou ajuste manual."));
        }
        Serial.println(F("[INFO] [RTC] RTC DS3231 inicializado."));
        return true;
    }
    _rtcDisponivel = false;
    fonteTempoAtual = FONTE_MILLIS;
    Serial.println(F("[WARN] [RTC] RTC ausente. Usando millis() como fallback."));
    return false;
}

bool Relogio::sincronizarNTP()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(F("[WARN] [NTP] Sync NTP ignorado: Wi-Fi desconectado."));
        return false;
    }

    Serial.println(F("[INFO] [NTP] Sincronizando com servidores NTP..."));
    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC,
               NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

    struct tm timeinfo;
    unsigned long inicio = millis();
    bool ok = false;

    while (millis() - inicio < NTP_SYNC_TIMEOUT_MS)
    {
        if (getLocalTime(&timeinfo, 1000))
        {
            ok = true;
            break;
        }
        delay(200);
    }

    if (!ok)
    {
        Serial.println(F("[WARN] [NTP] Falha ao sincronizar (timeout). Mantendo fonte atual."));
        return false;
    }

    if (_rtcDisponivel)
    {
        DateTime agoraNTP(
            timeinfo.tm_year + 1900,
            timeinfo.tm_mon + 1,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
        _rtc.adjust(agoraNTP);
        Serial.println(F("[INFO] [NTP] RTC ajustado via NTP."));
        fonteTempoAtual = FONTE_RTC;
    }
    else
    {
        fonteTempoAtual = FONTE_NTP;
        Serial.println(F("[INFO] [NTP] Sem RTC. Fonte de tempo = NTP (Wi-Fi)."));
    }
    _ultimaSincronizacaoNTP = millis();
    return true;
}

void Relogio::talvezRessincronizar()
{
    if (WiFi.status() != WL_CONNECTED)
        return;
    if (_ultimaSincronizacaoNTP == 0)
        return;
    if (millis() - _ultimaSincronizacaoNTP < NTP_SYNC_INTERVAL_MS)
        return;
    Serial.println(F("[INFO] [NTP] Ressincronizacao periodica NTP."));
    sincronizarNTP();
}

String Relogio::timestamp()
{
    DateTime agora = _agora();
    if (agora.unixtime() > 0)
    {
        char buf[20];
        sprintf(buf, "%02d/%02d %02d:%02d:%02d",
                agora.day(), agora.month(),
                agora.hour(), agora.minute(), agora.second());
        return String(buf);
    }
    unsigned long ms = millis();
    unsigned long s = ms / 1000;
    unsigned long m = s / 60;
    unsigned long h = m / 60;
    s %= 60;
    m %= 60;
    char buf[12];
    sprintf(buf, "%02lu:%02lu:%02lu", h, m, s);
    return String(buf);
}

String Relogio::timestampISO()
{
    DateTime agora = _agora();
    if (agora.unixtime() > 0)
    {
        char buf[25];
        sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d",
                agora.year(), agora.month(), agora.day(),
                agora.hour(), agora.minute(), agora.second());
        return String(buf);
    }
    return String(millis());
}

String Relogio::fonteTempoStr() const
{
    switch (fonteTempoAtual)
    {
    case FONTE_NTP:
        return "NTP";
    case FONTE_RTC:
        return "RTC";
    case FONTE_MILLIS:
        return "MILLIS";
    default:
        return "UNKNOWN";
    }
}

bool Relogio::ajustarManual(int ano, int mes, int dia, int hora, int min, int seg)
{
    if (!_rtcDisponivel)
    {
        Serial.println(F("[WARN] [RTC] Ajuste manual ignorado: RTC ausente."));
        return false;
    }
    _rtc.adjust(DateTime(ano, mes, dia, hora, min, seg));
    fonteTempoAtual = FONTE_RTC;
    _ultimaSincronizacaoNTP = millis();
    Serial.println(F("[INFO] [RTC] RTC ajustado manualmente."));
    return true;
}

DateTime Relogio::_agora()
{
    if (_rtcDisponivel)
    {
        DateTime agora = _rtc.now();
        if (agora.year() >= 2020)
            return agora;
    }
    if (fonteTempoAtual == FONTE_NTP && WiFi.status() == WL_CONNECTED)
    {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 0))
        {
            return DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1,
                            timeinfo.tm_mday, timeinfo.tm_hour,
                            timeinfo.tm_min, timeinfo.tm_sec);
        }
    }
    return DateTime((uint32_t)0);
}