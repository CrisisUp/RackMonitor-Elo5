#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <vector>
#include "../../include/config.h"
#include "../../include/state.h"
#include "../relogio/relogio.h"
#include "../alarme/alarme.h"
#include "../porta/porta.h"
#include "../ldr/ldr.h"
#include "../ultrassonico/ultrassonico.h"
#include "../audio/audio.h"
#include "../../include/ring_buffer.h"

// Variáveis externas do main.cpp
extern Alarme alarme;
extern Porta porta;
extern LDR ldr;
extern Ultrassonico ultrassonico;
extern Relogio relogio;
extern Audio audio;
extern Preferences prefs;
extern RingBufferLog logBuffer;
extern float _ultimaTempValida;
extern float _ultimaUmidValida;
extern bool alertaTempAtivo;
extern bool criticoTempAtivo;
extern bool alertaUmidAtivo;
extern bool criticoUmidAtivo;
extern bool taxaTempAlerta;
extern EstadoAmbiental estadoAmbiental;
extern float limiarTempAlerta;
extern float limiarTempCritico;
extern float limiarUmidAlerta;
extern float limiarUmidCritico;
extern bool contadoresDirty;
extern bool _sensorDHTPerdido;

// Funções externas do main.cpp
void logSerial(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);
void salvarConfig();

class NotificadorMQTT
{
public:
    void begin();
    void update();
    bool estaConectado();
    bool publicarLog(const String &payload);

private:
    WiFiClient _wifiClient;
    PubSubClient _client{_wifiClient};
    unsigned long _ultimoTempoReconexao = 0;
    unsigned long _ultimoHeartbeat = 0;
    const unsigned long INTERVALO_HEARTBEAT_MS = 30000UL;
    EstadoSistema _ultimoEstadoPublicado = SEGURO;
    ModoVigilante _ultimoModoPublicado = ARMADO;

    bool _estavaConectado = false;
    bool _precisaPublicarInicial = true;
    bool _ultimoEstadoPortaPublicado = false;
    unsigned long _ultimoStatusForcado = 0;
    const unsigned long STATUS_RATE_LIMIT_MS = 10000UL;

    unsigned long _ultimoAmbientePublicado = 0;
    const unsigned long INTERVALO_AMBIENTE_MS = 30000UL;

    void _publicar(bool forcar = false);
    void _publicarAmbiente();
    void _tratarComando(char *topic, byte *payload, unsigned int length);
};