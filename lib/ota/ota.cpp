#include "ota.h"

void configurarOTA()
{
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]()
                       {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        logSerial("INFO", "OTA", "Atualizacao iniciada (" + type + ").");
        enviarLogSite("OTA: atualizacao iniciada (" + type + ")");
        audio.forcarSilencio(); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
        static unsigned long _ultimoLog = 0;
        if (millis() - _ultimoLog > 2000) {
            _ultimoLog = millis();
            int pct = (progress * 100) / total;
            logSerial("INFO", "OTA", "Progresso: " + String(pct) + "%");
        } });

    ArduinoOTA.onEnd([]()
                     {
        logSerial("INFO", "OTA", "Atualizacao concluida. Reiniciando...");
        enviarLogSite("OTA: atualizacao concluida com sucesso"); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
        String motivo = "desconhecido";
        if (error == OTA_AUTH_ERROR)         motivo = "autenticacao";
        else if (error == OTA_BEGIN_ERROR)   motivo = "inicio";
        else if (error == OTA_CONNECT_ERROR) motivo = "conexao";
        else if (error == OTA_RECEIVE_ERROR) motivo = "recepcao";
        else if (error == OTA_END_ERROR)     motivo = "finalizacao";
        logSerial("CRIT", "OTA", "Erro na atualizacao: " + motivo);
        enviarLogSite("OTA: erro na atualizacao (" + motivo + ")"); });

    ArduinoOTA.begin();
    logSerial("INFO", "OTA", "Servico OTA ativo em " + String(OTA_HOSTNAME) + ":" + String(OTA_PORT) + " (senha configurada).");
}