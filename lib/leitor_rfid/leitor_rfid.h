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

    MFRC522 &getRFID() { return _rfid; }

private:
    MFRC522 _rfid;
};