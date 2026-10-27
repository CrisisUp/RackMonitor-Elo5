#pragma once
#include <Arduino.h>

// =========================================================================
// ENUMS DE ESTADO
// =========================================================================
enum ModoVigilante
{
    ARMADO,
    DESARMADO
};

enum EstadoSistema
{
    SEGURO,
    AVISO_SALA,
    APROXIMACAO_RACK,
    EMERGENCIA_RACK,
    INVASAO_SALA,
    ACESSO_RFID
};

enum EstadoAmbiental
{
    AMB_OK,
    AMB_ALERTA,
    AMB_CRITICO
};

enum FonteTempo
{
    FONTE_NTP,
    FONTE_RTC,
    FONTE_MILLIS
};

enum EventoBotao
{
    BOTAO_NENHUM,
    BOTAO_CURTO,
    BOTAO_DUPLO,
    BOTAO_LONGO
};

// =========================================================================
// STRUCTS DE DADOS
// =========================================================================
struct EventoRFID
{
    bool novo = false;
    bool valido = false;
    String uid = "";
};

struct LeiturasSensores
{
    bool movimento = false;
    bool luzAcesa = false;
    bool portaAberta = false;
    float distancia = 999.0f;
};