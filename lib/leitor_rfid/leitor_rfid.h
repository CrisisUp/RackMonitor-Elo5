#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "../../include/config.h"
#include "../../include/state.h"

class LeitorRFID
{
public:
    LeitorRFID();
    void begin();
    EventoRFID ler();
    bool estaPronto();

private:
    Adafruit_PN532 _nfc;
};
