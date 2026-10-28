#pragma once
#include <Arduino.h>
#include "../../include/config.h"
#include "../../include/state.h"

void logSerial(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);

extern float limiarTempAlerta;
extern float limiarTempCritico;
extern float limiarUmidAlerta;
extern float limiarUmidCritico;
extern bool alertaTempAtivo;
extern bool criticoTempAtivo;
extern bool alertaUmidAtivo;
extern bool criticoUmidAtivo;
extern bool taxaTempAlerta;
extern EstadoAmbiental estadoAmbiental;
extern unsigned long _ultimaLeituraValidaDHT;
extern bool _sensorDHTPerdido;
extern float _tempAnteriorTaxa;
extern unsigned long _tempoAnteriorTaxa;

void verificarAmbiente(float temp, float umid);