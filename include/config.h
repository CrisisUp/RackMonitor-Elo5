#pragma once
#include <cstdint>
#include "secrets.h"

// =========================================================================
// MAPEAMENTO DE HARDWARE
// =========================================================================
namespace Pins
{
    // --- RFID (PN532 via I2C) ---
    constexpr uint8_t RFID_SDA = 21;
    constexpr uint8_t RFID_SCL = 22;

    constexpr uint8_t PIR = 27;
    constexpr uint8_t TRIG = 17;
    constexpr uint8_t ECHO = 16;
    constexpr uint8_t LDR_AO = 34;

    constexpr uint8_t BOTAO = 14;
    constexpr uint8_t SWITCH = 25;

    constexpr uint8_t BUZZER_E = 15;
    constexpr uint8_t BUZZER_D = 2;

    constexpr uint8_t LED_R = 32;
    constexpr uint8_t LED_G = 33;
    constexpr uint8_t LED_B = 12;

    constexpr uint8_t BOTAO_PANICO = 26;

    // --- DHT22 ---
    constexpr uint8_t DHT_PIN = 13;
}

// =========================================================================
// CONFIGURAÇÕES DE HARDWARE E FILTROS
// =========================================================================
constexpr uint32_t PWM_FREQ = 5000;
constexpr uint8_t PWM_RESOLUTION = 8;
constexpr uint8_t LARGURA_TELA = 128;
constexpr uint8_t ALTURA_TELA = 64;
constexpr uint8_t ENDERECO_OLED = 0x3C;

constexpr int LDR_LIMIAR_ACENDE = 2200;
constexpr int LDR_LIMIAR_APAGA = 1800;
constexpr unsigned long DEBOUNCE_LDR_MS = 800;

constexpr int US_AMOSTRAS = 5;
constexpr float US_DIST_MAX_VALIDA = 400.0f;

constexpr unsigned long DEBOUNCE_ESTADO_MS = 300;
constexpr unsigned long DEBOUNCE_BOTAO_MS = 250;
constexpr unsigned long JANELA_CLIQUE_DUPLO_MS = 400;
constexpr unsigned long DURACAO_CLIQUE_LONGO_MS = 3000;
constexpr unsigned long DEBOUNCE_PORTA_MS = 200;
constexpr unsigned long DURACAO_AVISO_BLOQUEIO_MS = 4000;

// =========================================================================
// DHT22
// =========================================================================
// O tipo DHT22 vem da biblioteca DHT sensor library (enum DHT22).
// Para evitar conflito, definimos aqui como constante e usamos no sensor_dht.cpp.
// (Se preferir, use #define DHT_TYPE DHT22 no sensor_dht.h.)
constexpr uint8_t DHT_TYPE = 22; // 22 = DHT22 (enum da lib)
constexpr unsigned long DHT_INTERVALO_MIN_MS = 2000UL;

// =========================================================================
// LIMITES AMBIENTAIS (DEFAULT)
// =========================================================================
constexpr float TEMP_ALERTA_C_DEFAULT = 30.0f;
constexpr float TEMP_CRITICO_C_DEFAULT = 35.0f;
constexpr float UMID_ALERTA_PCT_DEFAULT = 65.0f;
constexpr float UMID_CRITICO_PCT_DEFAULT = 80.0f;

constexpr float TEMP_HISTERESE = 2.0f;
constexpr float UMID_HISTERESE = 5.0f;

constexpr float TAXA_ALERTA_C_POR_MIN = 1.0f;
constexpr unsigned long INTERVALO_AMOSTRA_TAXA_MS = 60000UL;

// =========================================================================
// CONFIGURAÇÃO DE REDE E PROTOCOLOS
// =========================================================================

const char *const MQTT_TOPIC_STATUS = "rack_monitor_ABCD1234/status";
const char *const MQTT_TOPIC_LWT = "rack_monitor_ABCD1234/status/lwt";
const char *const MQTT_TOPIC_CMD = "rack_monitor_ABCD1234/comando";
const char *const MQTT_TOPIC_LOGS = "rack_monitor_ABCD1234/logs";
const char *const MQTT_TOPIC_HB = "rack_monitor_ABCD1234/heartbeat";
const char *const MQTT_TOPIC_AMBIENTE = "rack_monitor_ABCD1234/ambiente";
const char *const MQTT_TOPIC_CMD_AMBIENTE = "rack_monitor_ABCD1234/comando/ambiente";

const unsigned long MQTT_RETRY_MS = 5000;
const char *const MQTT_RESET_TOKEN = "rack2026";

// =========================================================================
// NTP
// =========================================================================
const char *const NTP_SERVER_1 = "pool.ntp.org";
const char *const NTP_SERVER_2 = "time.google.com";
const char *const NTP_SERVER_3 = "time.cloudflare.com";
const long NTP_GMT_OFFSET_SEC = -3 * 3600;
const int NTP_DAYLIGHT_OFFSET_SEC = 0;
const unsigned long NTP_SYNC_INTERVAL_MS = 6UL * 60UL * 60UL * 1000UL;
const unsigned long NTP_SYNC_TIMEOUT_MS = 15000UL;

// =========================================================================
// OTA
// =========================================================================
const char *const OTA_HOSTNAME = "rack-ABCD1234";
const char *const OTA_PASSWORD = "rack@2026#ota";
const uint16_t OTA_PORT = 3232;

// =========================================================================
// DASHBOARD LOCAL
// =========================================================================
const uint16_t WEB_PORT = 8080;
const unsigned long SSE_KEEPALIVE_MS = 15000UL;