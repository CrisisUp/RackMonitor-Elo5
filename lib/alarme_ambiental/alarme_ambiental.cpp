#include "alarme_ambiental.h"
#include "../audio/audio.h" // Necessário para audio.forcarSilencio()

// Declara a instância global do audio (está no main.cpp)
extern Audio audio;

void verificarAmbiente(float temp, float umid)
{
    // Watchdog: se o DHT falhar por muito tempo, zera as flags
    if (temp == -999.0f || umid == -999.0f)
    {
        if (_ultimaLeituraValidaDHT > 0 &&
            (millis() - _ultimaLeituraValidaDHT) > 10000UL)
        {
            if (!_sensorDHTPerdido)
            {
                _sensorDHTPerdido = true;
                alertaTempAtivo = false;
                criticoTempAtivo = false;
                alertaUmidAtivo = false;
                criticoUmidAtivo = false;
                taxaTempAlerta = false;
                if (estadoAmbiental != AMB_OK)
                {
                    estadoAmbiental = AMB_OK;
                    audio.forcarSilencio();
                }
                logSerial("WARN", "AMBIENTE", "DHT22 sem leitura ha 10s. Estado ambiental resetado.");
                enviarLogSite("Sensor DHT22 sem leitura. Monitoramento ambiental pausado.");
            }
        }
        return;
    }

    _ultimaLeituraValidaDHT = millis();
    if (_sensorDHTPerdido)
    {
        _sensorDHTPerdido = false;
        logSerial("INFO", "AMBIENTE", "DHT22 voltou a responder. Retomando monitoramento.");
        enviarLogSite("Sensor DHT22 restabelecido.");
    }

    // Taxa de subida de temperatura
    unsigned long agora = millis();
    if (_tempoAnteriorTaxa == 0)
    {
        _tempAnteriorTaxa = temp;
        _tempoAnteriorTaxa = agora;
    }
    else if (agora - _tempoAnteriorTaxa >= INTERVALO_AMOSTRA_TAXA_MS)
    {
        float dtMin = (agora - _tempoAnteriorTaxa) / 60000.0f;
        float taxa = (temp - _tempAnteriorTaxa) / dtMin;

        if (!taxaTempAlerta && taxa >= TAXA_ALERTA_C_POR_MIN)
        {
            taxaTempAlerta = true;
            logSerial("CRIT", "AMBIENTE",
                      "SUBIDA RAPIDA: " + String(taxa, 2) + " C/min (limiar " +
                          String(TAXA_ALERTA_C_POR_MIN, 2) + " C/min)");
            enviarLogSite("CRITICO: Temperatura subindo " + String(taxa, 2) + " C/min!");
        }
        else if (taxaTempAlerta &&
                 (temp < limiarTempAlerta || taxa < (TAXA_ALERTA_C_POR_MIN * 0.5f)))
        {
            taxaTempAlerta = false;
            logSerial("INFO", "AMBIENTE",
                      "Taxa de subida normalizada: " + String(taxa, 2) + " C/min");
        }

        _tempAnteriorTaxa = temp;
        _tempoAnteriorTaxa = agora;
    }

    if (taxaTempAlerta && temp < limiarTempAlerta)
    {
        taxaTempAlerta = false;
        logSerial("INFO", "AMBIENTE",
                  "Taxa de subida resetada (temperatura segura: " + String(temp, 1) + "C).");
    }

    // Temperatura — ALERTA
    if (!alertaTempAtivo && temp >= limiarTempAlerta)
    {
        alertaTempAtivo = true;
        logSerial("WARN", "AMBIENTE",
                  "Temperatura alta: " + String(temp, 1) + "C (limiar " +
                      String(limiarTempAlerta, 1) + "C)");
        enviarLogSite("ALERTA: Temperatura do rack em " + String(temp, 1) + "C");
    }
    else if (alertaTempAtivo && temp <= (limiarTempAlerta - TEMP_HISTERESE))
    {
        alertaTempAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Temperatura normalizada: " + String(temp, 1) + "C");
        enviarLogSite("Temperatura do rack normalizada.");
    }

    // Temperatura — CRÍTICO
    if (!criticoTempAtivo && temp >= limiarTempCritico)
    {
        criticoTempAtivo = true;
        logSerial("CRIT", "AMBIENTE",
                  "TEMPERATURA CRITICA: " + String(temp, 1) + "C!");
        enviarLogSite("CRITICO: Temperatura do rack em " + String(temp, 1) + "C!");
    }
    else if (criticoTempAtivo && temp <= (limiarTempCritico - TEMP_HISTERESE))
    {
        criticoTempAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Temperatura critica normalizada: " + String(temp, 1) + "C");
        enviarLogSite("Temperatura critica normalizada.");
    }

    // Umidade — ALERTA
    if (!alertaUmidAtivo && umid >= limiarUmidAlerta)
    {
        alertaUmidAtivo = true;
        logSerial("WARN", "AMBIENTE",
                  "Umidade alta: " + String(umid, 1) + "% (limiar " +
                      String(limiarUmidAlerta, 1) + "%)");
        enviarLogSite("ALERTA: Umidade do rack em " + String(umid, 1) + "%");
    }
    else if (alertaUmidAtivo && umid <= (limiarUmidAlerta - UMID_HISTERESE))
    {
        alertaUmidAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Umidade normalizada: " + String(umid, 1) + "%");
        enviarLogSite("Umidade do rack normalizada.");
    }

    // Umidade — CRÍTICO
    if (!criticoUmidAtivo && umid >= limiarUmidCritico)
    {
        criticoUmidAtivo = true;
        logSerial("CRIT", "AMBIENTE",
                  "UMIDADE CRITICA: " + String(umid, 1) + "% (risco de condensacao)");
        enviarLogSite("CRITICO: Umidade do rack em " + String(umid, 1) + "%!");
    }
    else if (criticoUmidAtivo && umid <= (limiarUmidCritico - UMID_HISTERESE))
    {
        criticoUmidAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Umidade critica normalizada: " + String(umid, 1) + "%");
        enviarLogSite("Umidade critica normalizada.");
    }

    // Estado consolidado
    EstadoAmbiental novoEstado;
    if (criticoTempAtivo || criticoUmidAtivo || taxaTempAlerta)
    {
        novoEstado = AMB_CRITICO;
    }
    else if (alertaTempAtivo || alertaUmidAtivo)
    {
        novoEstado = AMB_ALERTA;
    }
    else
    {
        novoEstado = AMB_OK;
    }

    if (novoEstado != estadoAmbiental)
    {
        const char *nomes[] = {"OK", "ALERTA", "CRITICO"};
        logSerial(novoEstado == AMB_OK ? "INFO" : novoEstado == AMB_ALERTA ? "WARN"
                                                                           : "CRIT",
                  "AMBIENTE",
                  String("Estado ambiental: ") + nomes[estadoAmbiental] +
                      " -> " + nomes[novoEstado]);

        if (estadoAmbiental != AMB_OK && novoEstado == AMB_OK)
        {
            audio.forcarSilencio();
        }

        estadoAmbiental = novoEstado;
    }
}