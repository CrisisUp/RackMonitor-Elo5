#pragma once
#include <Arduino.h>
#include "../../include/config.h"
#include "../../include/ring_buffer.h"
#include "../relogio/relogio.h"
#include "../notificador_mqtt/notificador_mqtt.h"

extern Relogio relogio;
extern RingBufferLog logBuffer;
extern NotificadorMQTT notificador;

void logSerial(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);