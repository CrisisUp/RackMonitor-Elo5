#include "leitor_rfid.h"

LeitorRFID::LeitorRFID() : _rfid(Pins::RFID_SS, Pins::RFID_RST) {}

void LeitorRFID::begin()
{
    SPI.begin();
    _rfid.PCD_Init();
}

EventoRFID LeitorRFID::ler()
{
    EventoRFID ev;
    if (!_rfid.PICC_IsNewCardPresent() || !_rfid.PICC_ReadCardSerial())
        return ev;

    String uid = "";
    for (uint8_t i = 0; i < _rfid.uid.size; i++)
    {
        if (_rfid.uid.uidByte[i] < 0x10)
            uid += "0";
        uid += String(_rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    ev.novo = true;
    ev.uid = uid;
    ev.valido = false;

    _rfid.PICC_HaltA();
    _rfid.PCD_StopCrypto1();
    return ev;
}