#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "../../include/config.h"
#include "../relogio/relogio.h"
#include "../audio/audio.h"

// Funções externas do main.cpp
void logSerial(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);

extern Relogio relogio;
extern Audio audio;

void configurarOTA();