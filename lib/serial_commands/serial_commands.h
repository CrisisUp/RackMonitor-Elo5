#pragma once
#include <Arduino.h>
#include <vector>
#include "../../include/config.h"
#include "../../include/state.h"
#include "../../include/ring_buffer.h"
#include "../alarme/alarme.h"
#include "../relogio/relogio.h"
#include "../nvs/nvs.h"
#include "../porta/porta.h"
#include "../notificador_mqtt/notificador_mqtt.h"
#include "../audio/audio.h"

// Funções externas do main.cpp
void logSerial(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);

// Instâncias e variáveis globais do main.cpp
extern Alarme alarme;
extern Relogio relogio;
extern Porta porta;
extern Audio audio;
extern NotificadorMQTT notificador;
extern RingBufferLog logBuffer;
extern bool contadoresDirty;
extern float limiarTempAlerta;
extern float limiarTempCritico;
extern float limiarUmidAlerta;
extern float limiarUmidCritico;
extern float _ultimaTempValida;
extern float _ultimaUmidValida;
extern std::vector<String> cartoesAutorizados;

void processarComandoSerial(String cmd);