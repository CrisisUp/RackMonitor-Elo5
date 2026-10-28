#pragma once
#include "arduino_stubs.h"

// ==========================================
// ESTRUTURA E FUNÇÃO
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

inline String jsonEscape(const String &in)
{
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++)
    {
        char c = in[i];
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if ((uint8_t)c < 0x20)
            {
                char buf[7];
                snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t)c);
                out += buf;
            }
            else
            {
                out += c;
            }
        }
    }
    return out;
}