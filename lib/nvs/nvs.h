#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "../../include/config.h"
#include "../../include/state.h"
#include "../alarme/alarme.h"

// Variáveis externas do main.cpp
extern Preferences prefs;
extern Alarme alarme;
extern std::vector<String> cartoesAutorizados;
extern float limiarTempAlerta;
extern float limiarTempCritico;
extern float limiarUmidAlerta;
extern float limiarUmidCritico;

// Funções e constantes do main.cpp
void logSerial(const String &nivel, const String &tag, const String &mensagem);
extern const String CARTAO_AUTORIZADO_1;
extern const String CARTAO_AUTORIZADO_2;

// Funções do NVS
void salvarConfig();
void carregarCartoes();
void carregarConfig();