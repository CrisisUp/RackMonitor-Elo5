#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "../../include/config.h"
#include "../../include/state.h"

class LeitorRFID
{
public:
    LeitorRFID();
    void begin();
    EventoRFID ler();

private:
    MFRC522 _rfid;
};