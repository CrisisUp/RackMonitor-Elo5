#include "log_central.h"
#include "../../include/json_builder.h"

void logSerial(const String &nivel, const String &tag, const String &mensagem)
{
    Serial.print("[");
    Serial.print(relogio.timestamp());
    Serial.print("] [");
    Serial.print(nivel);
    Serial.print("]");
    if (nivel.length() == 4)
        Serial.print(" ");
    Serial.print(" [");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(mensagem);

    uint8_t n = NIVEL_INFO;
    if (nivel == "WARN")
        n = NIVEL_WARN;
    else if (nivel == "ALERT")
        n = NIVEL_ALERT;
    else if (nivel == "CRIT")
        n = NIVEL_CRIT;

    char buf[HIST_MSG_MAX];
    snprintf(buf, sizeof(buf), "[%s] %s", tag.c_str(), mensagem.c_str());
    logBuffer.push(n, buf);
}

void enviarLogSite(const String &mensagem)
{
    if (notificador.estaConectado())
    {
        String payload = "{\"ts\":\"" + relogio.timestamp() +
                         "\",\"msg\":\"" + jsonEscape(mensagem) + "\"}";
        if (!notificador.publicarLog(payload))
        {
            Serial.println(F("[WARN] Fila MQTT cheia, log descartado."));
        }
    }
}