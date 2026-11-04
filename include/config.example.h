#pragma once
#include <cstdint>

namespace Pins
{ /* ... mantenha igual ... */
}

// ... (mantenha todas as constantes de hardware, filtros, limites, OTA, web, etc.) ...

// =========================================================================
// CONFIGURAÇÃO DE REDE E PROTOCOLOS (SUBSTITUA PELOS SEUS DADOS)
// =========================================================================
const char *const WIFI_SSID = "SUA_REDE_WIFI";
const char *const WIFI_PASSWORD = "SUA_SENHA_WIFI";
const char *const MQTT_BROKER = "broker.emqx.io"; // Ou seu broker local
const int MQTT_PORT = 1883;

const char *const MQTT_TOPIC_STATUS = "SEU_TOPICO/status";
// ... (todos os outros tópicos) ...

const unsigned long MQTT_RETRY_MS = 5000;
const char *const MQTT_RESET_TOKEN = "SEU_TOKEN_RESET";

// ... (NTP, OTA, Web) ...

const char *const OTA_PASSWORD = "SUA_SENHA_OTA";