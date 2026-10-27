#pragma once

#ifdef NATIVE_TEST
// ==========================================
// MOCKS PARA O AMBIENTE NATIVE (PC)
// ==========================================
#include <string>
#include <cstdio>
#include <cstring>

// Simula a classe String do Arduino usando std::string
class String
{
public:
    std::string s;
    String() {}
    String(const char *c) : s(c) {}
    String(const std::string &str) : s(str) {}
    String(float f, int decimals)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.*f", decimals, f);
        s = buf;
    }
    String(int i) : s(std::to_string(i)) {}
    String(unsigned int i) : s(std::to_string(i)) {}
    String(long l) : s(std::to_string(l)) {}

    String &operator+=(const String &other)
    {
        s += other.s;
        return *this;
    }
    String &operator+=(const char *c)
    {
        s += c;
        return *this;
    }
    bool operator==(const String &other) const { return s == other.s; }

    size_t length() const { return s.length(); }
    char operator[](size_t i) const { return s[i]; }
    const char *c_str() const { return s.c_str(); }

    // Busca substring (usado nos testes)
    int indexOf(const String &str) const
    {
        size_t pos = s.find(str.s);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
};

#else
// ==========================================
// AMBIENTE ESP32 (Arduino real)
// ==========================================
#include <Arduino.h>
#endif

// ==========================================
// ESTRUTURA E FUNÇÃO (comum aos dois)
// ==========================================
struct DadosJSON
{
    String estado;
    String modo;
    String ts;
    String time_source;
    bool porta;
    bool luz;
    float dist_cm;
    bool ativo;
    String ip;
    String uid;
    unsigned int acessos;
    unsigned int invasoes;
    unsigned int rearmes;
    bool alerta_temp;
    bool critico_temp;
    bool alerta_umid;
    bool critico_umid;
    bool taxa_temp_alerta;
    String ambiente;
    float temp_c;
    float umid_pct;
};

inline String gerarJSON(const DadosJSON &d)
{
    String p = "{\"estado\":\"";
    p += d.estado;
    p += "\",\"modo\":\"";
    p += d.modo;
    p += "\",\"ts\":\"";
    p += d.ts;
    p += "\",\"time_source\":\"";
    p += d.time_source;
    p += "\",\"porta\":";
    p += (d.porta ? "true" : "false");
    p += ",\"luz\":";
    p += (d.luz ? "true" : "false");
    p += ",\"dist_cm\":";
    if (d.dist_cm < 0.0f)
        p += "null";
    else
        p += String(d.dist_cm, 1);
    p += ",\"ativo\":";
    p += (d.ativo ? "true" : "false");
    p += ",\"ip\":\"";
    p += d.ip;
    p += "\",\"uid\":\"";
    p += d.uid;
    p += "\",\"acessos\":";
    p += String(d.acessos);
    p += ",\"invasoes\":";
    p += String(d.invasoes);
    p += ",\"rearmes\":";
    p += String(d.rearmes);
    p += ",\"alerta_temp\":";
    p += (d.alerta_temp ? "true" : "false");
    p += ",\"critico_temp\":";
    p += (d.critico_temp ? "true" : "false");
    p += ",\"alerta_umid\":";
    p += (d.alerta_umid ? "true" : "false");
    p += ",\"critico_umid\":";
    p += (d.critico_umid ? "true" : "false");
    p += ",\"taxa_temp_alerta\":";
    p += (d.taxa_temp_alerta ? "true" : "false");
    p += ",\"ambiente\":\"";
    p += d.ambiente;
    p += "\",\"temp_c\":";
    p += String(d.temp_c, 1);
    p += ",\"umid_pct\":";
    p += String(d.umid_pct, 1);
    p += "}";
    return p;
}