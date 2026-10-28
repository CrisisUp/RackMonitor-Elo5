#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../relogio/relogio.h"
#include "../../include/config.h"
#include "../../include/state.h"

// Declara que essas variáveis existem no main.cpp
extern EstadoAmbiental estadoAmbiental;
extern float _ultimaTempValida;
extern float _ultimaUmidValida;
extern Relogio relogio;

class DisplayOLED
{
public:
    DisplayOLED(); // <-- ADICIONE ESTA LINHA
    void begin();
    Adafruit_SSD1306 &getDisplay() { return _display; }
    void mostrarStatus(EstadoSistema estado, ModoVigilante modo,
                       const LeiturasSensores &s,
                       const String &uidDesarme, bool cartaoOk,
                       unsigned int contAberturas, unsigned int contInvasoes,
                       const String &uidRearme);
    void mostrarDesarmado(unsigned long restanteMs, const String &uid,
                          unsigned int movManutencao);
    void mostrarBloqueio(const String &motivo, unsigned int totalBloqueios,
                         const String &uid);

private:
    void _desenharIcone(int tipo, int x, int y);
    Adafruit_SSD1306 _display;
};