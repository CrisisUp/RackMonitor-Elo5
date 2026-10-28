/**
 * @file main.cpp
 * @brief Sistema de Monitoramento de Rack (ESP32).
 *
 * Versão 7.4 - Modularizado, testado nativamente e com documentação embutida.
 *
 * Arquitetura:
 * Sensores -> Eventos -> Alarme (única fonte de verdade) -> Atuadores
 * (Display, LED, Áudio) + MQTT (observador) + OTA (manutenção) +
 * RingBufferLog (auditoria em RAM) + Dashboard Local (contingência).
 *
 * Este arquivo é o ponto de entrada do firmware. Ele apenas:
 *  - Inicializa os módulos no setup()
 *  - Orquestra o ciclo no loop()
 *
 * Toda a lógica está encapsulada nas bibliotecas em lib/.
 */

// =========================================================================
// CHANGELOG
// =========================================================================
//   v7.4 - MODULARIZAÇÃO E TESTES:
//          [1] Todas as classes movidas para lib/ (hardware, lógica, comunicação).
//          [2] main.cpp reduzido a um orquestrador puro.
//          [3] 12 testes nativos passando (alarme, json, ring buffer).
//          [4] Documentação de arquitetura preservada.
//   v7.3 - RING BUFFER + HIST:
//          [1] Buffer circular de 100 eventos em RAM (sem String, sem malloc).
//          [2] Struct EventoLog { uint32_t ms; uint8_t nivel; char msg[64]; }.
//          [3] logSerial() agora alimenta o buffer automaticamente.
//          [4] Comandos HIST / HIST_CLEAR / HIST_JSON.
//          [5] Push com noInterrupts()/interrupts() (compatível com ISR do pânico).
//   v7.2 - OTA (ArduinoOTA) + persistência do modo na NVS.
//   v7.1 - NTP + RTC Sync + fallback millis com flag no JSON.
//   v7.0 - Tópico ambiente dedicado + comando ambiente segregado + pânico ISR.
//   v6.5 - Cache DHT, logs não-bloqueantes, OLED com ambiente, reset robusto.
//   v6.4 - Correções anteriores (dupla publicação, rate limit, LWT, NVS dirty).
//   v6.3 - LWT (Last Will), Retain flag, NVS, comandos remotos MQTT.
//   v6.2 - Integração Wi-Fi + MQTT.
//   v6.1 - Doxygen; correção duplo update() do botão; ordem init I2C.
//   v6.0 - Refactor: classes encapsuladas, struct de sensores, máquina isolada.
//   v5.0 - Versão original em arquivo único.

// =========================================================================
// MAPEAMENTO DE HARDWARE - ESP32 DevKit
// =========================================================================
// [ OCUPADOS ]
//  GPIO | Módulo       | Função                    | Fio           | Observação
//  -----|--------------|---------------------------|---------------|----------------------
//   0   | LedRGB       | Canal B                   | Azul          | Strapping (boot) — ATENÇÃO
//   2   | Audio        | Buzzer D                  | Rosa          | Strapping (boot)
//   4   | LeitorRFID   | RST                       | Laranja       |
//   5   | LeitorRFID   | SS (SDA)                  | Rosa          |
//  13   | DHT22        | Dados (SDA)               | Amarelo       | Sensor temp/umid
//  14   | Botao        | Pushbutton                | Laranja       | INPUT_PULLUP
//  15   | Audio        | Buzzer E                  | Roxo Escuro   | Strapping (boot)
//  16   | Ultrassonico | ECHO                      | Laranja       |
//  17   | Ultrassonico | TRIG                      | Verde         |
//  18   | LeitorRFID   | SCK                       | Cinza         |
//  19   | LeitorRFID   | MISO                      | Verde         |
//  21   | DisplayOLED  | I2C SDA (OLED + RTC)      | Verde         | Wire.begin(21,22)
//  22   | DisplayOLED  | I2C SCL (OLED + RTC)      | Verde Claro   | Wire.begin(21,22)
//  23   | LeitorRFID   | MOSI                      | Azul Claro    |
//  25   | Porta        | Switch                    | Verde         | INPUT_PULLDOWN
//  26   | BotaoPanico  | Pushbutton (v7.0)         | Branco        | INPUT_PULLUP + ISR
//  27   | PIR          | Movimento                 | Verde Claro   |
//  32   | LedRGB       | Canal R                   | Marrom        |
//  33   | LedRGB       | Canal G                   | Verde         |
//  34   | LDR          | Leitura Analógica (AO)    | Verde         | Input-only
//
// [ LIVRES PARA EXPANSÃO ]
//  - GPIO 12 : Livre (Strapping — cuidado no boot)
//  - GPIO 35 : Livre (Apenas Entrada Analógica)
//  - GPIO 36 : Livre (Apenas Entrada Analógica)
//  - GPIO 39 : Livre (Apenas Entrada Analógica)
//
// [ PROIBIDOS ]
//  - GPIO 6 a 11 : Conectados à memória Flash (NUNCA USAR)
//  - GPIO 1 e 3  : Usados pela Serial USB (TX0/RX0)
// =========================================================================

/**
 * @brief Arquitetura de Software e Regras de Ouro.
 *
 * REGRAS DE OURO
 * 1. Sensores NÃO decidem nada — só medem e filtram.
 * 2. Atuadores NÃO leem sensores — só consomem estado.
 * 3. Alarme NÃO acessa hardware direto — recebe struct/eventos.
 * 4. Eventos são "fatos", não "ações".
 * 5. Toda transição é logada com nível (INFO/ALERT/CRIT).
 * 6. loop() só orquestra.
 * 7. MQTT é OBSERVADOR do Alarme.
 * 8. NVS é persistência PASSIVA.
 */

// =========================================================================
// INCLUDES
// =========================================================================

// Bibliotecas do sistema
#include <Wire.h>
#include <vector>

// Configurações e estruturas globais
#include "config.h"
#include "state.h"
#include "ring_buffer.h"
#include "json_builder.h"

// Bibliotecas do projeto (lib/)
#include "ldr.h"
#include "pir.h"
#include "porta.h"
#include "ultrassonico.h"
#include "sensor_dht.h"
#include "led_rgb.h"
#include "audio.h"
#include "display_oled.h"
#include "relogio.h"
#include "botao.h"
#include "botao_panico.h"
#include "leitor_rfid.h"
#include "alarme.h"
#include "notificador_mqtt.h"
#include "nvs.h"
#include "ota.h"
#include "test_mode.h"
#include "serial_commands.h"
#include "log_central.h"
#include "alarme_ambiental.h"

// =========================================================================
// VARIÁVEIS DE ESTADO (mudam durante a execução)
// =========================================================================
float limiarTempAlerta = TEMP_ALERTA_C_DEFAULT;
float limiarTempCritico = TEMP_CRITICO_C_DEFAULT;
float limiarUmidAlerta = UMID_ALERTA_PCT_DEFAULT;
float limiarUmidCritico = UMID_CRITICO_PCT_DEFAULT;

bool alertaTempAtivo = false;
bool criticoTempAtivo = false;
bool alertaUmidAtivo = false;
bool criticoUmidAtivo = false;
bool taxaTempAlerta = false;

// Watchdog do DHT22
unsigned long _ultimaLeituraValidaDHT = 0;
bool _sensorDHTPerdido = false;
constexpr unsigned long TIMEOUT_DHT_MS = 10000UL;

// Cache das últimas leituras válidas do DHT
float _ultimaTempValida = -999.0f;
float _ultimaUmidValida = -999.0f;

EstadoAmbiental estadoAmbiental = AMB_OK;
FonteTempo fonteTempoAtual = FONTE_MILLIS;

#define ALARME_SONORO_AMBIENTAL 0

float _tempAnteriorTaxa = -999.0f;
unsigned long _tempoAnteriorTaxa = 0;

bool contadoresDirty = false;

// Cartões autorizados (padrão)
const String CARTAO_AUTORIZADO_1 = "01020304";
const String CARTAO_AUTORIZADO_2 = "55667788";
std::vector<String> cartoesAutorizados;

// =========================================================================
// INSTÂNCIAS GLOBAIS
// =========================================================================

// Persistência e auditoria
Preferences prefs;
RingBufferLog logBuffer;

// Sensores e atuadores
LDR ldr;
PIR pir;
Porta porta;
Ultrassonico ultrassonico;
SensorDHT dht;
LedRGB led;
Audio audio;
DisplayOLED oled;
Relogio relogio;

// Botões
Botao botao;
BotaoPanico botaoPanico;

// RFID, Alarme e MQTT
LeitorRFID leitorRFID;
Alarme alarme;
NotificadorMQTT notificador;

// =========================================================================
// VARIÁVEIS DE CONTROLE DO LOOP
// =========================================================================
unsigned long tempoUltimoBip = 0;
unsigned long tempoUltimoPisca = 0;
bool estadoPiscaLED = false;

unsigned long tempoUltimoSalvamento = 0;
constexpr unsigned long INTERVALO_SALVAMENTO_MS = 60000UL;

String bufferSerial = "";

// =========================================================================
// SETUP
// =========================================================================

/**
 * @brief Inicialização do sistema.
 *
 * Ordem de inicialização:
 * 1. Hardware base (LED, I2C, display, relógio)
 * 2. Sensores (LDR, US, porta, PIR, botões)
 * 3. RFID e áudio
 * 4. DHT22
 * 5. Ring buffer
 * 6. Carregar config da NVS
 * 7. Banner e logs
 * 8. MQTT, OTA e NTP
 */
void setup()
{
    Serial.begin(115200);
    delay(200);

    // 1. Hardware base
    led.begin();
    led.selfTest();

    Wire.begin(21, 22);
    relogio.begin();
    oled.begin();

    // 2. Sensores
    ldr.begin();
    ultrassonico.begin();
    porta.begin();
    pir.begin();
    botao.begin();
    botaoPanico.begin();

    // 3. RFID e áudio
    leitorRFID.begin();
    audio.begin();

    // 4. DHT22
    dht.begin();

    // 5. Ring buffer (antes do primeiro log)
    logBuffer.begin();

    // 6. Configurações da NVS
    carregarConfig();
    carregarCartoes();

    // 7. Banner
    Serial.println();
    Serial.println(F("================================================="));
    Serial.println(F("  SISTEMA DE MONITORAMENTO DE RACK - v7.4"));
    Serial.println(F("  Modularizado + Testes Nativos"));
    Serial.println(F("================================================="));

    logSerial("INFO", "SISTEMA", "Sistema inicializado. Modo: " + String(alarme.modo == ARMADO ? "ARMADO" : "DESARMADO"));
    logSerial("INFO", "RFID", "Cartao 1: " + CARTAO_AUTORIZADO_1);
    logSerial("INFO", "RFID", "Cartao 2: " + CARTAO_AUTORIZADO_2);
    logSerial("INFO", "RTC", relogio.disponivel() ? "DS3231 ativo." : "Usando millis.");
    logSerial("INFO", "VIGILANTE", "Timeout: " + String(alarme.timeoutDesarme / 60000) + " min");
    logSerial("INFO", "VIGILANTE", "Rearme BLOQUEADO se porta aberta.");
    logSerial("INFO", "PANICO", "Botao no GPIO 26 (ISR).");
    logSerial("INFO", "OTA", "Hostname: " + String(OTA_HOSTNAME) + " | Porta: " + String(OTA_PORT));
    logSerial("INFO", "HIST", "Buffer de " + String(HIST_TAMANHO) + " eventos (" + String(sizeof(EventoLog) * HIST_TAMANHO) + " bytes em RAM).");
    logSerial("INFO", "SISTEMA", "Digite HELP no Serial para comandos.");
    Serial.println(F("-------------------------------------------------"));

    // 8. Resetar variáveis ambientais
    alertaTempAtivo = false;
    criticoTempAtivo = false;
    alertaUmidAtivo = false;
    criticoUmidAtivo = false;
    taxaTempAlerta = false;
    estadoAmbiental = AMB_OK;
    _ultimaLeituraValidaDHT = 0;
    _sensorDHTPerdido = false;
    _ultimaTempValida = -999.0f;
    _ultimaUmidValida = -999.0f;

    // 9. MQTT
    notificador.begin();

    // 10. OTA (se Wi-Fi conectado)
    if (WiFi.status() == WL_CONNECTED)
    {
        configurarOTA();
    }
    else
    {
        logSerial("WARN", "OTA", "Wi-Fi offline no boot.");
    }

    // 11. NTP (se Wi-Fi conectado)
    if (WiFi.status() == WL_CONNECTED)
    {
        delay(1000);
        if (relogio.sincronizarNTP())
        {
            logSerial("INFO", "NTP", "Sync OK. Fonte: " + relogio.fonteTempoStr());
        }
        else
        {
            logSerial("WARN", "NTP", "Sync falhou. Fonte: " + relogio.fonteTempoStr());
        }
    }
    else
    {
        logSerial("WARN", "NTP", "Wi-Fi offline. Fonte: " + relogio.fonteTempoStr());
    }
}

// =========================================================================
// LOOP PRINCIPAL
// =========================================================================

/**
 * @brief Loop principal do sistema.
 *
 * Ordem de execução:
 * 1. OTA (prioridade máxima)
 * 2. Comandos Serial
 * 3. Sensores contínuos (LDR, US, porta, PIR)
 * 4. Eventos (botão, pânico, RFID)
 * 5. Máquina de estados (Alarme)
 * 6. Alarme ambiental (DHT)
 * 7. Ações pendentes
 * 8. Display
 * 9. Áudio
 * 10. LED
 * 11. MQTT
 * 12. Ressincronização NTP
 * 13. Salvamento periódico
 */
void loop()
{
    // 0. OTA
    ArduinoOTA.handle();

    // 1. Comandos Serial
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (bufferSerial.length() > 0)
            {
                processarComandoSerial(bufferSerial);
                bufferSerial = "";
            }
        }
        else
        {
            bufferSerial += c;
        }
    }

    // 2. Sensores contínuos
    ldr.update();
    ultrassonico.update();
    porta.update();
    pir.update();

    // 3. Eventos
    EventoBotao evBotao = botao.update();
    if (botaoPanico.consumir())
    {
        alarme.dispararPanico("BOTAO_FISICO");
    }
    EventoRFID evRFID = leitorRFID.ler();

    // 4. Snapshot
    LeiturasSensores s;
    s.movimento = pir.detectaMovimento();
    s.luzAcesa = ldr.estaAcesa();
    s.portaAberta = porta.estaAberta();
    s.distancia = ultrassonico.distancia();

    // 5. Máquina de estados
    alarme.update(s, evRFID, evBotao, porta, audio);

    // 5.1. Alarme ambiental
    float tempAtual = dht.temperatura();
    float umidAtual = dht.umidade();
    if (tempAtual != -999.0f)
        _ultimaTempValida = tempAtual;
    if (umidAtual != -999.0f)
        _ultimaUmidValida = umidAtual;
    verificarAmbiente(tempAtual, umidAtual);

    // 6. Ações pendentes
    if (alarme.audioPararPendente)
    {
        alarme.audioPararPendente = false;
        audio.parar();
    }
    if (alarme.resetContadoresPendente)
    {
        alarme.resetContadoresPendente = false;
        alarme.contadorAberturasPorta = 0;
        alarme.contadorAproximacoes = 0;
        alarme.contadorAcessosValidos = 0;
        alarme.contadorMovimentosManutencao = 0;
        alarme.contadorDesarmes = 0;
        salvarConfig();
    }
    if (alarme.modoTestePendente)
    {
        alarme.modoTestePendente = false;
        executarModoTeste();
    }

    // 7. Display
    if (alarme.modo == DESARMADO)
    {
        unsigned long decorrido = millis() - alarme.tempoInicioDesarme;
        unsigned long restante = (alarme.timeoutDesarme > decorrido)
                                     ? (alarme.timeoutDesarme - decorrido)
                                     : 0;
        oled.mostrarDesarmado(restante, alarme.uidUltimoDesarme,
                              alarme.contadorMovimentosManutencao);
    }
    else if (alarme.bloqueioAtivo())
    {
        oled.mostrarBloqueio(alarme.motivoBloqueio,
                             alarme.contadorRearmesBloqueados,
                             alarme.uidUltimoDesarme);
    }
    else
    {
        oled.mostrarStatus(alarme.estadoAtual, alarme.modo, s,
                           alarme.uidUltimoDesarme, alarme.ultimoCartaoValido,
                           alarme.contadorAberturasPorta,
                           alarme.contadorInvasoes,
                           alarme.uidUltimoRearme);
    }

    // 8. Áudio
    EstadoSistema estadoParaAudio = (alarme.modo == ARMADO) ? alarme.estadoAtual : SEGURO;
    bool bloqueioParaAudio = (alarme.modo == ARMADO) ? alarme.bloqueioAtivo() : false;
    audio.tocar(estadoParaAudio, bloqueioParaAudio, estadoAmbiental);

    // 9. LED
    led.atualizar(alarme.modo, alarme.estadoAtual,
                  alarme.bloqueioAtivo(), tempoUltimoPisca, estadoPiscaLED);

    // 10. MQTT
    notificador.update();

    // 11. Ressincronização NTP
    relogio.talvezRessincronizar();

    // 12. Salvamento periódico
    if (contadoresDirty && millis() - tempoUltimoSalvamento > INTERVALO_SALVAMENTO_MS)
    {
        tempoUltimoSalvamento = millis();
        contadoresDirty = false;
        salvarConfig();
    }

    delay(20);
}