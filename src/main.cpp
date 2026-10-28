/**
 * @file RackMonitor.ino
 * @brief Sistema de Monitoramento de Rack (ESP32).
 *
 * Versão 7.3 - Ring buffer de eventos + HIST (debug pós-incidente).
 *
 * Arquitetura:
 * Sensores -> Eventos -> Alarme (única fonte de verdade) -> Atuadores
 * (Display, LED, Áudio) + MQTT (observador) + OTA (manutenção) +
 * RingBufferLog (auditoria em RAM).
 */

// =========================================================================
// CHANGELOG
// =========================================================================
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
//  26   | BotaoPanico  | Pushbutton (v7.0)         | Branco.       | INPUT_PULLUP + ISR
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
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <vector>
#include <DHT.h>
#include <math.h>
#include <time.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "state.h"
#include "ring_buffer.h"
#include "json_builder.h"
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
constexpr unsigned long TIMEOUT_DHT_MS = 10000UL; // Mantido aqui, se não estiver no config.h

// Cache das últimas leituras válidas do DHT
float _ultimaTempValida = -999.0f;
float _ultimaUmidValida = -999.0f;

EstadoAmbiental estadoAmbiental = AMB_OK;
FonteTempo fonteTempoAtual = FONTE_MILLIS;

#define ALARME_SONORO_AMBIENTAL 0

float _tempAnteriorTaxa = -999.0f;
unsigned long _tempoAnteriorTaxa = 0;

bool contadoresDirty = false;

const String CARTAO_AUTORIZADO_1 = "01020304";
const String CARTAO_AUTORIZADO_2 = "55667788";

std::vector<String> cartoesAutorizados;

// =========================================================================
// PERIFÉRICOS
// =========================================================================

Preferences prefs;

// =========================================================================
// [v7.3] RING BUFFER DE EVENTOS
// =========================================================================
// Buffer circular em RAM. O(n) no dump, O(1) no push.
// Protegido contra ISR via noInterrupts()/interrupts() no push — o botão
// de pânico pode chamar registrarEvento() de dentro da ISR sem corromper
// o índice circular.
// =========================================================================

RingBufferLog logBuffer;

// =========================================================================
// LOG CENTRALIZADO — protótipos ANTES de Relogio
// =========================================================================

void logSerial(const String &nivel, const String &tag, const String &mensagem);
void registrarEvento(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);

// =========================================================================
// MÓDULO: RELÓGIO (v7.1 — NTP + RTC + fallback millis)
// =========================================================================

Relogio relogio;

// =========================================================================
// LOG CENTRALIZADO — implementação (usa relogio.timestamp())
// =========================================================================

/**
 * @brief Log na Serial E no ring buffer. Este é o ponto único de entrada
 *        para logs de eventos. Todo o código deve chamar isto (via
 *        registrarEvento) em vez de Serial.println direto.
 */
void logSerial(const String &nivel, const String &tag, const String &mensagem)
{
    Serial.print("[");
    Serial.print(relogio.timestamp());
    Serial.print("] [");
    Serial.print(nivel);
    Serial.print("]");
    if (nivel.length() == 4)
        Serial.print(" ");
    Serial.print(" [");
    Serial.print(tag);
    Serial.print("] ");
    Serial.println(mensagem);

    // [v7.3] Alimenta o ring buffer (exceto se o buffer ainda não foi iniciado)
    // Mapeia o nível string para o enum numérico
    uint8_t n = NIVEL_INFO;
    if (nivel == "WARN")
        n = NIVEL_WARN;
    else if (nivel == "ALERT")
        n = NIVEL_ALERT;
    else if (nivel == "CRIT")
        n = NIVEL_CRIT;

    // Formato compacto no buffer: "[TAG] mensagem"
    char buf[HIST_MSG_MAX];
    snprintf(buf, sizeof(buf), "[%s] %s", tag.c_str(), mensagem.c_str());
    logBuffer.push(n, buf);
}

// Wrapper semântico — quem chama registrarEvento está ciente de que é
// um evento de auditoria (não um log interno qualquer).
void registrarEvento(const String &nivel, const String &tag, const String &mensagem)
{
    logSerial(nivel, tag, mensagem);
}

LDR ldr;

Ultrassonico ultrassonico;

Porta porta;

PIR pir;

Botao botao;

BotaoPanico botaoPanico;

LeitorRFID leitorRFID;

LedRGB led;

Audio audio;

DisplayOLED oled;

SensorDHT dht;

Alarme alarme;

NotificadorMQTT notificador;

// =========================================================================
// ALARME AMBIENTAL
// =========================================================================

void _verificarAmbiente(float temp, float umid)
{
    if (temp == -999.0f || umid == -999.0f)
    {
        if (_ultimaLeituraValidaDHT > 0 &&
            (millis() - _ultimaLeituraValidaDHT) > 10000UL)
        {
            if (!_sensorDHTPerdido)
            {
                _sensorDHTPerdido = true;
                alertaTempAtivo = false;
                criticoTempAtivo = false;
                alertaUmidAtivo = false;
                criticoUmidAtivo = false;
                taxaTempAlerta = false;
                if (estadoAmbiental != AMB_OK)
                {
                    estadoAmbiental = AMB_OK;
                    audio.forcarSilencio();
                }
                logSerial("WARN", "AMBIENTE", "DHT22 sem leitura ha 10s. Estado ambiental resetado.");
                enviarLogSite("Sensor DHT22 sem leitura. Monitoramento ambiental pausado.");
            }
        }
        return;
    }

    _ultimaLeituraValidaDHT = millis();
    if (_sensorDHTPerdido)
    {
        _sensorDHTPerdido = false;
        logSerial("INFO", "AMBIENTE", "DHT22 voltou a responder. Retomando monitoramento.");
        enviarLogSite("Sensor DHT22 restabelecido.");
    }

    unsigned long agora = millis();
    if (_tempoAnteriorTaxa == 0)
    {
        _tempAnteriorTaxa = temp;
        _tempoAnteriorTaxa = agora;
    }
    else if (agora - _tempoAnteriorTaxa >= INTERVALO_AMOSTRA_TAXA_MS)
    {
        float dtMin = (agora - _tempoAnteriorTaxa) / 60000.0f;
        float taxa = (temp - _tempAnteriorTaxa) / dtMin;

        if (!taxaTempAlerta && taxa >= TAXA_ALERTA_C_POR_MIN)
        {
            taxaTempAlerta = true;
            logSerial("CRIT", "AMBIENTE",
                      "SUBIDA RAPIDA: " + String(taxa, 2) + " C/min (limiar " +
                          String(TAXA_ALERTA_C_POR_MIN, 2) + " C/min)");
            enviarLogSite("CRITICO: Temperatura subindo " + String(taxa, 2) + " C/min!");
        }
        else if (taxaTempAlerta &&
                 (temp < limiarTempAlerta || taxa < (TAXA_ALERTA_C_POR_MIN * 0.5f)))
        {
            taxaTempAlerta = false;
            logSerial("INFO", "AMBIENTE",
                      "Taxa de subida normalizada: " + String(taxa, 2) + " C/min");
        }

        _tempAnteriorTaxa = temp;
        _tempoAnteriorTaxa = agora;
    }

    if (taxaTempAlerta && temp < limiarTempAlerta)
    {
        taxaTempAlerta = false;
        logSerial("INFO", "AMBIENTE",
                  "Taxa de subida resetada (temperatura segura: " + String(temp, 1) + "C).");
    }

    if (!alertaTempAtivo && temp >= limiarTempAlerta)
    {
        alertaTempAtivo = true;
        logSerial("WARN", "AMBIENTE",
                  "Temperatura alta: " + String(temp, 1) + "C (limiar " +
                      String(limiarTempAlerta, 1) + "C)");
        enviarLogSite("ALERTA: Temperatura do rack em " + String(temp, 1) + "C");
    }
    else if (alertaTempAtivo && temp <= (limiarTempAlerta - TEMP_HISTERESE))
    {
        alertaTempAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Temperatura normalizada: " + String(temp, 1) + "C");
        enviarLogSite("Temperatura do rack normalizada.");
    }

    if (!criticoTempAtivo && temp >= limiarTempCritico)
    {
        criticoTempAtivo = true;
        logSerial("CRIT", "AMBIENTE",
                  "TEMPERATURA CRITICA: " + String(temp, 1) + "C!");
        enviarLogSite("CRITICO: Temperatura do rack em " + String(temp, 1) + "C!");
    }
    else if (criticoTempAtivo && temp <= (limiarTempCritico - TEMP_HISTERESE))
    {
        criticoTempAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Temperatura critica normalizada: " + String(temp, 1) + "C");
        enviarLogSite("Temperatura critica normalizada.");
    }

    if (!alertaUmidAtivo && umid >= limiarUmidAlerta)
    {
        alertaUmidAtivo = true;
        logSerial("WARN", "AMBIENTE",
                  "Umidade alta: " + String(umid, 1) + "% (limiar " +
                      String(limiarUmidAlerta, 1) + "%)");
        enviarLogSite("ALERTA: Umidade do rack em " + String(umid, 1) + "%");
    }
    else if (alertaUmidAtivo && umid <= (limiarUmidAlerta - UMID_HISTERESE))
    {
        alertaUmidAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Umidade normalizada: " + String(umid, 1) + "%");
        enviarLogSite("Umidade do rack normalizada.");
    }

    if (!criticoUmidAtivo && umid >= limiarUmidCritico)
    {
        criticoUmidAtivo = true;
        logSerial("CRIT", "AMBIENTE",
                  "UMIDADE CRITICA: " + String(umid, 1) + "% (risco de condensacao)");
        enviarLogSite("CRITICO: Umidade do rack em " + String(umid, 1) + "%!");
    }
    else if (criticoUmidAtivo && umid <= (limiarUmidCritico - UMID_HISTERESE))
    {
        criticoUmidAtivo = false;
        logSerial("INFO", "AMBIENTE",
                  "Umidade critica normalizada: " + String(umid, 1) + "%");
        enviarLogSite("Umidade critica normalizada.");
    }

    EstadoAmbiental novoEstado;
    if (criticoTempAtivo || criticoUmidAtivo || taxaTempAlerta)
    {
        novoEstado = AMB_CRITICO;
    }
    else if (alertaTempAtivo || alertaUmidAtivo)
    {
        novoEstado = AMB_ALERTA;
    }
    else
    {
        novoEstado = AMB_OK;
    }

    if (novoEstado != estadoAmbiental)
    {
        const char *nomes[] = {"OK", "ALERTA", "CRITICO"};
        logSerial(novoEstado == AMB_OK ? "INFO" : novoEstado == AMB_ALERTA ? "WARN"
                                                                           : "CRIT",
                  "AMBIENTE",
                  String("Estado ambiental: ") + nomes[estadoAmbiental] +
                      " -> " + nomes[novoEstado]);

        if (estadoAmbiental != AMB_OK && novoEstado == AMB_OK)
        {
            audio.forcarSilencio();
        }

        estadoAmbiental = novoEstado;
    }

#if ALARME_SONORO_AMBIENTAL
    bool segurancaCritica = (alarme.estadoAtual == EMERGENCIA_RACK ||
                             alarme.estadoAtual == INVASAO_SALA);
    if (!segurancaCritica)
    {
        static unsigned long _ultimoBipAmbiental = 0;
        unsigned long agoraBip = millis();

        if (estadoAmbiental == AMB_CRITICO && agoraBip - _ultimoBipAmbiental >= 3000)
        {
            tone(Pins::BUZZER_D, 400, 200);
            _ultimoBipAmbiental = agoraBip;
        }
        else if (estadoAmbiental == AMB_ALERTA && agoraBip - _ultimoBipAmbiental >= 8000)
        {
            tone(Pins::BUZZER_E, 700, 150);
            _ultimoBipAmbiental = agoraBip;
        }
    }
#endif
}

// =========================================================================
// ENVIO DE LOG PARA O SITE
// =========================================================================

void enviarLogSite(const String &mensagem)
{
    if (notificador.estaConectado())
    {
        String payload = "{\"ts\":\"" + relogio.timestamp() +
                         "\",\"msg\":\"" + jsonEscape(mensagem) + "\"}";
        if (!notificador.publicarLog(payload))
        {
            Serial.println(F("[WARN] Fila MQTT cheia, log descartado."));
        }
    }
}

// =========================================================================
// COMANDOS SERIAL
// =========================================================================
String bufferSerial = "";

// =========================================================================
// MODO TESTE
// =========================================================================

// =========================================================================
// SETUP
// =========================================================================

void setup()
{
    Serial.begin(115200);
    delay(200);

    led.begin();
    led.selfTest();

    Wire.begin(21, 22);
    relogio.begin();
    oled.begin();

    ldr.begin();
    ultrassonico.begin();
    porta.begin();
    pir.begin();
    botao.begin();
    botaoPanico.begin();

    leitorRFID.begin();
    audio.begin();

    dht.begin();

    logBuffer.begin(); // [v7.3] Inicializa o ring buffer ANTES do primeiro log

    carregarConfig();
    carregarCartoes();

    Serial.println();
    Serial.println(F("================================================="));
    Serial.println(F("  SISTEMA DE MONITORAMENTO DE RACK - v7.3"));
    Serial.println(F("  + Ring Buffer + HIST (auditoria em RAM)"));
    Serial.println(F("  + OTA (ArduinoOTA)"));
    Serial.println(F("  + NTP + RTC Sync + fallback millis"));
    Serial.println(F("  + Tópico ambiente dedicado"));
    Serial.println(F("  + Comandos de ambiente segregados"));
    Serial.println(F("  + Botão de pânico com ISR (GPIO 26)"));
    Serial.println(F("================================================="));
    logSerial("INFO", "SISTEMA", "Sistema inicializado. Modo: " + String(alarme.modo == ARMADO ? "ARMADO" : "DESARMADO"));
    logSerial("INFO", "RFID", "Cartao 1 Autorizado: " + CARTAO_AUTORIZADO_1);
    logSerial("INFO", "RFID", "Cartao 2 Autorizado: " + CARTAO_AUTORIZADO_2);
    logSerial("INFO", "RTC", relogio.disponivel() ? "RTC DS3231 ativo." : "RTC ausente (usando millis).");
    logSerial("INFO", "VIGILANTE", "Timeout de desarme: " + String(alarme.timeoutDesarme / 60000) + " min");
    logSerial("INFO", "VIGILANTE", "Politica: rearme BLOQUEADO se porta estiver aberta.");
    logSerial("INFO", "PANICO", "Botao de panico no GPIO 26 (ISR, pull-up interno).");
    logSerial("INFO", "OTA", "Hostname: " + String(OTA_HOSTNAME) + " | Porta: " + String(OTA_PORT));
    logSerial("INFO", "HIST", "Buffer de " + String(HIST_TAMANHO) + " eventos (" + String(sizeof(EventoLog) * HIST_TAMANHO) + " bytes em RAM).");
    logSerial("INFO", "FILTROS",
              "LDR: " + String(LDR_LIMIAR_APAGA) + "-" + String(LDR_LIMIAR_ACENDE) +
                  " deb " + String(DEBOUNCE_LDR_MS) + "ms | US media: " + String(US_AMOSTRAS));
    logSerial("INFO", "FILTROS",
              "Botao: deb " + String(DEBOUNCE_BOTAO_MS) + "ms | duplo <" +
                  String(JANELA_CLIQUE_DUPLO_MS) + "ms | longo >" + String(DURACAO_CLIQUE_LONGO_MS) + "ms");
    logSerial("INFO", "FILTROS", "Porta: deb " + String(DEBOUNCE_PORTA_MS) + "ms");
    logSerial("INFO", "SISTEMA", "Digite HELP no Serial para comandos disponiveis.");
    Serial.println(F("-------------------------------------------------"));

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

    notificador.begin();

    if (WiFi.status() == WL_CONNECTED)
    {
        configurarOTA();
    }
    else
    {
        logSerial("WARN", "OTA", "Wi-Fi offline no boot. OTA indisponivel ate reconectar.");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        delay(1000);
        if (relogio.sincronizarNTP())
        {
            logSerial("INFO", "NTP", "Sincronizacao inicial OK. Fonte: " + relogio.fonteTempoStr());
        }
        else
        {
            logSerial("WARN", "NTP", "Sync inicial falhou. Fonte atual: " + relogio.fonteTempoStr());
        }
    }
    else
    {
        logSerial("WARN", "NTP", "Wi-Fi offline no boot. Fonte: " + relogio.fonteTempoStr() + " (tentara ressync no loop).");
    }
}

// =========================================================================
// LOOP PRINCIPAL
// =========================================================================
unsigned long tempoUltimoBip = 0;
unsigned long tempoUltimoPisca = 0;
bool estadoPiscaLED = false;

unsigned long tempoUltimoSalvamento = 0;
constexpr unsigned long INTERVALO_SALVAMENTO_MS = 60000UL;

void loop()
{
    // 0. OTA
    ArduinoOTA.handle();

    // 1. Serial
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
    _verificarAmbiente(tempAtual, umidAtual);

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

    // 8. Áudio com prioridade
    EstadoSistema estadoParaAudio = (alarme.modo == ARMADO) ? alarme.estadoAtual : SEGURO;
    bool bloqueioParaAudio = (alarme.modo == ARMADO) ? alarme.bloqueioAtivo() : false;
    audio.tocar(estadoParaAudio, bloqueioParaAudio, estadoAmbiental);

    // 9. LED
    led.atualizar(alarme.modo, alarme.estadoAtual,
                  alarme.bloqueioAtivo(), tempoUltimoPisca, estadoPiscaLED);

    // 10. MQTT
    notificador.update();

    // 10.1 Ressincronização periódica NTP
    relogio.talvezRessincronizar();

    // 11. Salvamento periódico apenas se dirty
    if (contadoresDirty && millis() - tempoUltimoSalvamento > INTERVALO_SALVAMENTO_MS)
    {
        tempoUltimoSalvamento = millis();
        contadoresDirty = false;
        salvarConfig();
    }

    delay(20);
}