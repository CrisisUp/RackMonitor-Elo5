#pragma once
#include <Arduino.h>
#include "../../include/config.h"
#include "../../include/state.h"
#include "../ldr/ldr.h"
#include "../ultrassonico/ultrassonico.h"
#include "../porta/porta.h"
#include "../pir/pir.h"
#include "../sensor_dht/sensor_dht.h"
#include "../led_rgb/led_rgb.h"
#include "../display_oled/display_oled.h"
#include "../leitor_rfid/leitor_rfid.h"

// Funções externas do main.cpp
void logSerial(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);

// Instâncias globais
extern LDR ldr;
extern Ultrassonico ultrassonico;
extern Porta porta;
extern PIR pir;
extern SensorDHT dht;
extern LedRGB led;
extern DisplayOLED oled;
extern LeitorRFID leitorRFID;

void executarModoTeste();