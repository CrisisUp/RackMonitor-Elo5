#include "leitor_rfid.h"
#include "../../include/config.h"

LeitorRFID::LeitorRFID() : _nfc(Pins::RFID_SDA, Pins::RFID_SCL) {}

void LeitorRFID::begin()
{
    Wire.begin(Pins::RFID_SDA, Pins::RFID_SCL);
    _nfc.begin();

    uint32_t versiondata = _nfc.getFirmwareVersion();
    if (!versiondata)
    {
        Serial.println("[RFID] ERRO: PN532 nao encontrado!");
        return;
    }
    Serial.println("[RFID] PN532 inicializado com sucesso.");
    _nfc.SAMConfig();
}

EventoRFID LeitorRFID::ler()
{
    EventoRFID ev;
    ev.novo = false;
    ev.valido = false;

    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0};
    uint8_t uidLength;

    // Timeout de 100ms para não bloquear o loop principal
    bool success = _nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);

    if (!success)
        return ev;

    // Converte UID para String hexadecimal (mesmo formato do MFRC522 antigo)
    String uidStr = "";
    for (uint8_t i = 0; i < uidLength; i++)
    {
        if (uid[i] < 0x10)
            uidStr += "0";
        uidStr += String(uid[i], HEX);
    }
    uidStr.toUpperCase();

    ev.novo = true;
    ev.uid = uidStr;
    ev.valido = false;

    return ev;
}

// Adicionado: verifica se o PN532 está respondendo
bool LeitorRFID::estaPronto()
{
    return _nfc.getFirmwareVersion() != 0;
}