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
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, -1);
RTC_DS3231 rtc;
MFRC522 rfid(Pins::RFID_SS, Pins::RFID_RST);

bool rtcDisponivel = false;

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

// =========================================================================
// MÓDULO: NVS
// =========================================================================

void salvarConfig()
{
    prefs.begin("rack", false);
    prefs.putFloat("limiar", alarme.limiarDistancia);
    prefs.putULong("timeout", alarme.timeoutDesarme);
    prefs.putUInt("invasoes", alarme.contadorInvasoes);
    prefs.putUInt("aberturas", alarme.contadorAberturasPorta);
    prefs.putUInt("acessos", alarme.contadorAcessosValidos);
    prefs.putUInt("desarmes", alarme.contadorDesarmes);
    prefs.putUInt("rearmes", alarme.contadorRearmesBloqueados);
    prefs.putFloat("t_alert", limiarTempAlerta);
    prefs.putFloat("t_crit", limiarTempCritico);
    prefs.putFloat("u_alert", limiarUmidAlerta);
    prefs.putFloat("u_crit", limiarUmidCritico);
    prefs.end();
    logSerial("INFO", "NVS", "Configuracoes salvas na flash.");
}

void carregarCartoes()
{
    cartoesAutorizados.clear();

    auto adicionarSeNovo = [](const String &uid)
    {
        for (const String &c : cartoesAutorizados)
            if (c == uid)
                return;
        cartoesAutorizados.push_back(uid);
    };

    prefs.begin("rack", true);

    adicionarSeNovo(CARTAO_AUTORIZADO_1);
    adicionarSeNovo(CARTAO_AUTORIZADO_2);

    int total = prefs.getInt("total_cards", 0);
    for (int i = 0; i < total; i++)
    {
        String chave = "card_" + String(i);
        String uid = prefs.getString(chave.c_str(), "");
        if (uid.length() > 0)
        {
            adicionarSeNovo(uid);
            logSerial("INFO", "NVS", "Cartao carregado: " + uid);
        }
    }
    prefs.end();
    logSerial("INFO", "NVS", "Total de cartoes carregados: " + String(cartoesAutorizados.size()));
}

void carregarConfig()
{
    prefs.begin("rack", true);
    alarme.limiarDistancia = prefs.getFloat("limiar", 50.0f);
    alarme.timeoutDesarme = prefs.getULong("timeout", 30UL * 60UL * 1000UL);
    alarme.contadorInvasoes = prefs.getUInt("invasoes", 0);
    alarme.contadorAberturasPorta = prefs.getUInt("aberturas", 0);
    alarme.contadorAcessosValidos = prefs.getUInt("acessos", 0);
    alarme.contadorDesarmes = prefs.getUInt("desarmes", 0);
    alarme.contadorRearmesBloqueados = prefs.getUInt("rearmes", 0);
    limiarTempAlerta = prefs.getFloat("t_alert", TEMP_ALERTA_C_DEFAULT);
    limiarTempCritico = prefs.getFloat("t_crit", TEMP_CRITICO_C_DEFAULT);
    limiarUmidAlerta = prefs.getFloat("u_alert", UMID_ALERTA_PCT_DEFAULT);
    limiarUmidCritico = prefs.getFloat("u_crit", UMID_CRITICO_PCT_DEFAULT);
    uint8_t modoSalvo = prefs.getUInt("modo", (uint8_t)ARMADO);
    alarme.modo = (modoSalvo == (uint8_t)DESARMADO) ? DESARMADO : ARMADO;
    prefs.end();
    logSerial("INFO", "NVS", "Configuracoes carregadas da flash.");
    logSerial("INFO", "NVS", "Modo restaurado: " + String(alarme.modo == ARMADO ? "ARMADO" : "DESARMADO"));
}

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
// HELPER JSON
// =========================================================================

String jsonEscape(const String &in)
{
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++)
    {
        char c = in[i];
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if ((uint8_t)c < 0x20)
            {
                char buf[7];
                snprintf(buf, sizeof(buf), "\\u%04x", (uint8_t)c);
                out += buf;
            }
            else
            {
                out += c;
            }
        }
    }
    return out;
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
// [v7.2] OTA — configuração e callbacks
// =========================================================================

void configurarOTA()
{
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]()
                       {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    logSerial("INFO", "OTA", "Atualizacao iniciada (" + type + ").");
    enviarLogSite("OTA: atualizacao iniciada (" + type + ")");
    audio.forcarSilencio(); });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
    static unsigned long _ultimoLog = 0;
    if (millis() - _ultimoLog > 2000) {
      _ultimoLog = millis();
      int pct = (progress * 100) / total;
      logSerial("INFO", "OTA", "Progresso: " + String(pct) + "%");
    } });

    ArduinoOTA.onEnd([]()
                     {
    logSerial("INFO", "OTA", "Atualizacao concluida. Reiniciando...");
    enviarLogSite("OTA: atualizacao concluida com sucesso"); });

    ArduinoOTA.onError([](ota_error_t error)
                       {
    String motivo = "desconhecido";
    if (error == OTA_AUTH_ERROR)         motivo = "autenticacao";
    else if (error == OTA_BEGIN_ERROR)   motivo = "inicio";
    else if (error == OTA_CONNECT_ERROR) motivo = "conexao";
    else if (error == OTA_RECEIVE_ERROR) motivo = "recepcao";
    else if (error == OTA_END_ERROR)     motivo = "finalizacao";
    logSerial("CRIT", "OTA", "Erro na atualizacao: " + motivo);
    enviarLogSite("OTA: erro na atualizacao (" + motivo + ")"); });

    ArduinoOTA.begin();
    logSerial("INFO", "OTA", "Servico OTA ativo em " + String(OTA_HOSTNAME) + ":" + String(OTA_PORT) + " (senha configurada).");
}

// =========================================================================
// COMANDOS SERIAL
// =========================================================================
String bufferSerial = "";

void processarComandoSerial(String cmd)
{
    cmd.trim();
    cmd.toUpperCase();
    logSerial("INFO", "CMD", "Comando recebido: " + cmd);

    if (cmd.startsWith("SET LIMIAR "))
    {
        float novo = cmd.substring(11).toFloat();
        if (novo >= 5 && novo <= 400)
        {
            alarme.limiarDistancia = novo;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar ajustado para " + String(novo) + " cm.");
        }
        else
        {
            logSerial("WARN", "CMD", "Valor fora da faixa (5-400 cm).");
        }
    }
    else if (cmd.startsWith("SET TIMEOUT "))
    {
        long minutos = cmd.substring(12).toInt();
        if (minutos >= 0 && minutos <= 1440)
        {
            alarme.timeoutDesarme = (unsigned long)minutos * 60UL * 1000UL;
            salvarConfig();
            logSerial("INFO", "CMD", "Timeout de desarme: " + String(minutos) + " min.");
            if (alarme.modo == DESARMADO)
            {
                alarme.tempoInicioDesarme = millis();
                alarme.aviso5minDisparado = false;
            }
        }
        else
        {
            logSerial("WARN", "CMD", "Timeout fora da faixa (0-1440 min).");
        }
    }
    else if (cmd.startsWith("SET TEMP_ALERTA "))
    {
        float v = cmd.substring(16).toFloat();
        if (v >= 15 && v <= 50)
        {
            limiarTempAlerta = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar temp alerta: " + String(v, 1) + "C");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (15-50).");
    }
    else if (cmd.startsWith("SET TEMP_CRIT "))
    {
        float v = cmd.substring(14).toFloat();
        if (v >= 20 && v <= 60)
        {
            limiarTempCritico = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar temp critico: " + String(v, 1) + "C");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (20-60).");
    }
    else if (cmd.startsWith("SET UMID_ALERTA "))
    {
        float v = cmd.substring(16).toFloat();
        if (v >= 30 && v <= 90)
        {
            limiarUmidAlerta = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar umid alerta: " + String(v, 1) + "%");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (30-90).");
    }
    else if (cmd.startsWith("SET UMID_CRIT "))
    {
        float v = cmd.substring(14).toFloat();
        if (v >= 40 && v <= 100)
        {
            limiarUmidCritico = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar umid critico: " + String(v, 1) + "%");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (40-100).");
    }
    else if (cmd == "STATUS")
    {
        Serial.println(F("----------- STATUS ATUAL -----------"));
        Serial.print(F("  Modo Vigilante:       "));
        Serial.println(alarme.nomeModo());
        Serial.print(F("  Estado Alarme:        "));
        Serial.println(alarme.nomeEstadoAtual());
        Serial.print(F("  Porta (filtrada):     "));
        Serial.println(porta.estaAberta() ? "ABERTA" : "fechada");
        Serial.print(F("  Timeout Desarme:      "));
        Serial.print(alarme.timeoutDesarme / 60000UL);
        Serial.println(F(" min"));
        if (alarme.modo == DESARMADO)
        {
            unsigned long dec = millis() - alarme.tempoInicioDesarme;
            unsigned long rst = (alarme.timeoutDesarme > dec) ? (alarme.timeoutDesarme - dec) / 1000UL : 0;
            Serial.print(F("  Rearme em:            "));
            Serial.print(rst);
            Serial.println(F(" s"));
            Serial.print(F("  UID Desarme:          "));
            Serial.println(alarme.uidUltimoDesarme);
        }
        else
        {
            Serial.print(F("  UID Ultimo Rearme:    "));
            Serial.println(alarme.uidUltimoRearme.length() ? alarme.uidUltimoRearme : "-");
        }
        Serial.print(F("  Contador Desarmes:    "));
        Serial.println(alarme.contadorDesarmes);
        Serial.print(F("  Rearmes Bloqueados:   "));
        Serial.println(alarme.contadorRearmesBloqueados);
        Serial.print(F("  Duracao ult. desarme: "));
        Serial.print(alarme.duracaoUltimoDesarme);
        Serial.println(F(" s"));
        Serial.print(F("  Limiar Distancia:     "));
        Serial.print(alarme.limiarDistancia);
        Serial.println(F(" cm"));
        Serial.print(F("  Aberturas da Porta:   "));
        Serial.println(alarme.contadorAberturasPorta);
        Serial.print(F("  Acessos RFID:         "));
        Serial.println(alarme.contadorAcessosValidos);
        Serial.print(F("  Invasoes:             "));
        Serial.println(alarme.contadorInvasoes);
        Serial.print(F("  Aproximacoes:         "));
        Serial.println(alarme.contadorAproximacoes);
        Serial.print(F("  Mov. em manutencao:   "));
        Serial.println(alarme.contadorMovimentosManutencao);
        Serial.print(F("  Limiares ambientais:  T["));
        Serial.print(limiarTempAlerta, 1);
        Serial.print(F("/"));
        Serial.print(limiarTempCritico, 1);
        Serial.print(F("C] U["));
        Serial.print(limiarUmidAlerta, 1);
        Serial.print(F("/"));
        Serial.print(limiarUmidCritico, 1);
        Serial.println(F("%]"));
        Serial.print(F("  Temp/umid atuais:     T="));
        Serial.print(_ultimaTempValida, 1);
        Serial.print(F("C U="));
        Serial.print(_ultimaUmidValida, 1);
        Serial.println(F("%"));
        Serial.print(F("  Fonte de tempo:       "));
        Serial.print(relogio.fonteTempoStr());
        Serial.print(F(" | Hora: "));
        Serial.println(relogio.timestamp());
        Serial.print(F("  Historico (buffer):   "));
        Serial.print(logBuffer.tamanho());
        Serial.print(F("/"));
        Serial.println(HIST_TAMANHO);
        Serial.print(F("  WiFi:                 "));
        Serial.println(WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado");
        Serial.print(F("  MQTT:                 "));
        Serial.println(notificador.estaConectado() ? "conectado" : "desconectado");
        Serial.print(F("  OTA:                  "));
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print(OTA_HOSTNAME);
            Serial.print(":");
            Serial.print(OTA_PORT);
            Serial.println(F(" (ativo)"));
        }
        else
        {
            Serial.println(F("(Wi-Fi offline)"));
        }
        Serial.println(F("-------------------------------------"));
    }
    else if (cmd == "OTA")
    {
        Serial.println(F("----------- STATUS OTA -----------"));
        Serial.print(F("  Hostname:  "));
        Serial.println(OTA_HOSTNAME);
        Serial.print(F("  Porta:     "));
        Serial.println(OTA_PORT);
        Serial.print(F("  Senha:     "));
        Serial.println(OTA_PASSWORD);
        Serial.print(F("  IP:        "));
        if (WiFi.status() == WL_CONNECTED)
            Serial.println(WiFi.localIP());
        else
            Serial.println(F("Wi-Fi offline"));
        Serial.print(F("  Ativo:     "));
        Serial.println(WiFi.status() == WL_CONNECTED ? "sim" : "nao (Wi-Fi offline)");
        Serial.println(F("-----------------------------------"));
        Serial.println(F("  No Arduino IDE: Ferramentas > Porta > rede"));
        Serial.println(F("  Selecione o dispositivo e faca upload normalmente."));
    }
    else if (cmd == "HIST")
    {
        logBuffer.dumpSerial();
    }
    else if (cmd == "HIST_CLEAR")
    {
        logBuffer.clear();
        logSerial("INFO", "CMD", "Comando HIST_CLEAR executado.");
    }
    else if (cmd == "HIST_JSON")
    {
        logBuffer.dumpJSON();
    }
    else if (cmd == "ARMAR")
    {
        if (alarme.modo == DESARMADO)
        {
            logSerial("INFO", "CMD", "Tentativa de ARMAR via Serial.");
            alarme.armarViaSerial(porta, audio);
            salvarConfig();
        }
        else
        {
            logSerial("WARN", "CMD", "Ja esta armado.");
        }
    }
    else if (cmd == "DESARMAR")
    {
        if (alarme.modo == ARMADO)
        {
            alarme.modo = DESARMADO;
            alarme.tempoInicioDesarme = millis();
            alarme.uidUltimoDesarme = "SERIAL";
            alarme.aviso5minDisparado = false;
            alarme.contadorDesarmes++;
            contadoresDirty = true;
            prefs.begin("rack", false);
            prefs.putUInt("modo", (uint8_t)DESARMADO);
            prefs.end();
            logSerial("INFO", "CMD", "Modo Vigilante DESARMADO via Serial.");
            salvarConfig();
        }
        else
        {
            logSerial("WARN", "CMD", "Ja esta desarmado.");
        }
    }
    else if (cmd == "TESTE")
    {
        logSerial("INFO", "CMD", "Executando modo teste via Serial.");
        alarme.modoTestePendente = true;
    }
    else if (cmd == "PANICO")
    {
        logSerial("INFO", "CMD", "Executando panico via Serial.");
        alarme.dispararPanico("SERIAL");
    }
    else if (cmd == "RESET")
    {
        alarme.contadorAberturasPorta = 0;
        alarme.contadorAproximacoes = 0;
        alarme.contadorAcessosValidos = 0;
        alarme.contadorMovimentosManutencao = 0;
        alarme.contadorDesarmes = 0;
        salvarConfig();
        logSerial("INFO", "CMD", "Contadores operacionais zerados (seguranca preservada).");
    }
    else if (cmd.startsWith("RTC "))
    {
        int ano, mes, dia, hora, min, seg;
        if (sscanf(cmd.c_str(), "RTC %d-%d-%d %d:%d:%d",
                   &ano, &mes, &dia, &hora, &min, &seg) == 6)
        {
            if (relogio.ajustarManual(ano, mes, dia, hora, min, seg))
            {
                logSerial("INFO", "RTC", "Ajuste manual OK. Fonte: " + relogio.fonteTempoStr());
            }
        }
        else
        {
            logSerial("WARN", "CMD", "Formato: RTC YYYY-MM-DD HH:MM:SS");
        }
    }
    else if (cmd == "RTC")
    {
        logSerial("INFO", "RTC", "Hora atual: " + relogio.timestamp() + " | Fonte: " + relogio.fonteTempoStr());
    }
    else if (cmd == "NTP")
    {
        if (relogio.sincronizarNTP())
        {
            logSerial("INFO", "NTP", "Sync manual OK. Fonte: " + relogio.fonteTempoStr());
        }
        else
        {
            logSerial("WARN", "NTP", "Sync manual falhou.");
        }
    }
    else if (cmd.startsWith("ADD_CARD "))
    {
        String novoUID = cmd.substring(9);
        novoUID.trim();
        if (novoUID.length() >= 8)
        {
            bool jaExiste = false;
            for (const String &c : cartoesAutorizados)
            {
                if (c == novoUID)
                {
                    jaExiste = true;
                    break;
                }
            }
            if (jaExiste)
            {
                logSerial("WARN", "CMD", "Cartao ja cadastrado: " + novoUID);
            }
            else
            {
                prefs.begin("rack", false);
                int total = prefs.getInt("total_cards", 0);
                String chave = "card_" + String(total);
                prefs.putString(chave.c_str(), novoUID);
                prefs.putInt("total_cards", total + 1);
                prefs.end();
                cartoesAutorizados.push_back(novoUID);
                logSerial("INFO", "CMD", "Cartao cadastrado na NVS: " + novoUID);
            }
        }
        else
        {
            logSerial("WARN", "CMD", "UID invalido. Use ADD_CARD <UID_HEX> (min 8 chars)");
        }
    }
    else if (cmd == "LIST_CARDS")
    {
        Serial.println(F("--- Cartoes Autorizados ---"));
        for (const String &card : cartoesAutorizados)
            Serial.println(card);
        Serial.println(F("---------------------------"));
    }
    else if (cmd == "HELP")
    {
        Serial.println(F("----------- COMANDOS DISPONIVEIS -----------"));
        Serial.println(F("  SET LIMIAR <5-400>      - Ajusta limiar ultrassonico (persiste)"));
        Serial.println(F("  SET TIMEOUT <0-1440>    - Timeout de desarme (min, persiste)"));
        Serial.println(F("  SET TEMP_ALERTA <15-50> - Limiar temp alerta (C, persiste)"));
        Serial.println(F("  SET TEMP_CRIT <20-60>   - Limiar temp critico (C, persiste)"));
        Serial.println(F("  SET UMID_ALERTA <30-90> - Limiar umid alerta (%, persiste)"));
        Serial.println(F("  SET UMID_CRIT <40-100>  - Limiar umid critico (%, persiste)"));
        Serial.println(F("  STATUS                  - Status completo"));
        Serial.println(F("  ARMAR                   - Forca ARMADO (se porta fechada)"));
        Serial.println(F("  DESARMAR                - Forca DESARMADO"));
        Serial.println(F("  TESTE                   - Modo teste"));
        Serial.println(F("  PANICO                  - Dispara panico manual"));
        Serial.println(F("  RTC                     - Mostra hora e fonte de tempo"));
        Serial.println(F("  RTC YYYY-MM-DD HH:MM:SS - Ajusta RTC manualmente"));
        Serial.println(F("  NTP                     - Forca sincronizacao NTP manual"));
        Serial.println(F("  OTA                     - Mostra status do servico OTA"));
        Serial.println(F("  HIST                    - Despeja historico de eventos (Serial)"));
        Serial.println(F("  HIST_JSON               - Despeja historico em JSON"));
        Serial.println(F("  HIST_CLEAR              - Limpa o historico de eventos"));
        Serial.println(F("  RESET                   - Zera contadores operacionais (preserva seguranca)"));
        Serial.println(F("  ADD_CARD <UID>          - Cadastra novo cartao na NVS"));
        Serial.println(F("  LIST_CARDS              - Lista cartoes autorizados"));
        Serial.println(F("  HELP                    - Lista comandos"));
        Serial.println(F("---------------------------------------------"));
        Serial.println(F("  BOTAO : curto=reset | longo=rearme | duplo=teste"));
        Serial.println(F("  PANICO: GPIO 26 (pushbutton para GND) - ISR, resposta <30ms"));
        Serial.println(F("  OTA   : Arduino IDE > Ferramentas > Porta > rede"));
        Serial.println(F("  HIST  : buffer circular de 100 eventos em RAM (perdido no reboot)"));
        Serial.println(F("  MQTT  : rack/status (pub) | rack/comando (sub seguranca)"));
        Serial.println(F("  MQTT  : rack/ambiente (pub retain 30s) | rack/comando/ambiente (sub)"));
        Serial.println(F("  MQTT  : rack/status/lwt | rack/logs | rack/heartbeat"));
        Serial.println(F("  MQTT_CMD seg: ARMAR | DESARMAR | STATUS | RESET <token> | TESTE"));
        Serial.println(F("  MQTT_CMD amb: SET_TEMP_ALERTA | SET_TEMP_CRIT | SET_UMID_ALERTA | SET_UMID_CRIT | PANICO | STATUS_AMB"));
        Serial.println(F("  NTP: pool.ntp.org | time.google.com | time.cloudflare.com"));
    }
    else
    {
        logSerial("WARN", "CMD", "Comando desconhecido. Digite HELP.");
    }
}

// =========================================================================
// MODO TESTE
// =========================================================================

void executarModoTeste()
{
    logSerial("INFO", "TESTE", "=== INICIANDO AUTODIAGNOSTICO v3 ===");
    enviarLogSite("TESTE: Iniciando autodiagnostico...");

    int ok = 0, falhas = 0;
    String resumoFalhas = "";

    if (display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED))
    {
        logSerial("INFO", "TESTE", "[OK] Display OLED (I2C) respondendo.");
        enviarLogSite("TESTE: OLED OK");
        ok++;
    }
    else
    {
        logSerial("CRIT", "TESTE", "[FALHA] Display OLED NAO responde.");
        enviarLogSite("TESTE: OLED FALHA");
        resumoFalhas += "OLED ";
        falhas++;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 20);
    display.println("MODO TESTE");
    display.setCursor(10, 40);
    display.println("Aguarde...");
    display.display();

    byte versao = rfid.PCD_ReadRegister(MFRC522::VersionReg);
    if (versao != 0x00 && versao != 0xFF)
    {
        logSerial("INFO", "TESTE", "[OK] RFID MFRC522 (v0x" + String(versao, HEX) + ").");
        enviarLogSite("TESTE: RFID OK");
        ok++;
    }
    else
    {
        logSerial("CRIT", "TESTE", "[FALHA] RFID MFRC522 NAO responde.");
        enviarLogSite("TESTE: RFID FALHA");
        resumoFalhas += "RFID ";
        falhas++;
    }

    float d = ultrassonico.distancia();
    if (d > 0.0f && d < US_DIST_MAX_VALIDA)
    {
        logSerial("INFO", "TESTE", "[OK] Ultrassonico: " + String(d, 1) + " cm.");
        enviarLogSite("TESTE: Ultrassonico OK (" + String(d, 1) + " cm)");
        ok++;
    }
    else
    {
        logSerial("CRIT", "TESTE", "[FALHA] Ultrassonico: " + String(d, 1) + " cm.");
        enviarLogSite("TESTE: Ultrassonico FALHA");
        resumoFalhas += "US ";
        falhas++;
    }

    int ldrRaw = ldr.leituraBruta();
    logSerial("INFO", "TESTE", "[INFO] LDR: " + String(ldrRaw) + " (" + String(ldr.estaAcesa() ? "ACESA" : "APAGADA") + ").");
    enviarLogSite("TESTE: LDR " + String(ldr.estaAcesa() ? "ACESA" : "APAGADA"));
    ok++;

    logSerial("INFO", "TESTE", "[INFO] Porta: " + String(porta.estaAberta() ? "ABERTA" : "fechada") + ".");
    enviarLogSite("TESTE: Porta " + String(porta.estaAberta() ? "ABERTA" : "FECHADA"));
    ok++;

    logSerial("INFO", "TESTE", "[INFO] PIR: " + String(pir.detectaMovimento() ? "ATIVO" : "inativo") + ".");
    enviarLogSite("TESTE: PIR " + String(pir.detectaMovimento() ? "ATIVO" : "INATIVO"));
    ok++;

    float t = dht.temperatura();
    float u = dht.umidade();
    if (t > -50.0f && t < 80.0f)
    {
        logSerial("INFO", "TESTE", "[OK] DHT22: " + String(t, 1) + "C / " + String(u, 1) + "%.");
        enviarLogSite("TESTE: DHT22 OK (" + String(t, 1) + "C / " + String(u, 1) + "%)");
        ok++;
    }
    else
    {
        logSerial("CRIT", "TESTE", "[FALHA] DHT22 sem leitura valida.");
        enviarLogSite("TESTE: DHT22 FALHA");
        resumoFalhas += "DHT22 ";
        falhas++;
    }

    logSerial("INFO", "TESTE", "[INFO] Testando buzzers...");
    enviarLogSite("TESTE: Testando Buzzers...");
    tone(Pins::BUZZER_E, 500, 150);
    delay(200);
    tone(Pins::BUZZER_E, 1000, 150);
    delay(200);
    tone(Pins::BUZZER_E, 2000, 150);
    delay(200);
    tone(Pins::BUZZER_D, 800, 300);
    delay(350);
    ok++;

    logSerial("INFO", "TESTE", "[INFO] Testando LED RGB...");
    enviarLogSite("TESTE: Testando LED RGB...");
    led.definirCor(255, 0, 0);
    delay(300);
    led.definirCor(0, 255, 0);
    delay(300);
    led.definirCor(0, 0, 255);
    delay(300);
    led.definirCor(255, 255, 255);
    delay(300);
    led.definirCor(0, 0, 0);
    ok++;

    String resumo = "TESTE CONCLUIDO: " + String(ok) + " OK";
    if (falhas > 0)
        resumo += ", " + String(falhas) + " FALHA(S): " + resumoFalhas;

    logSerial(falhas > 0 ? "WARN" : "INFO", "TESTE", resumo);
    enviarLogSite(resumo);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("TESTE CONCLUIDO");
    display.drawLine(0, 12, 128, 12, SSD1306_WHITE);
    display.setCursor(0, 20);
    display.print("OK:     ");
    display.println(ok);
    display.setCursor(0, 32);
    display.print("FALHAS: ");
    display.println(falhas);
    display.display();

    if (falhas == 0)
    {
        tone(Pins::BUZZER_E, 1500, 100);
        delay(120);
        tone(Pins::BUZZER_E, 2000, 150);
    }
    else
    {
        tone(Pins::BUZZER_D, 400, 500);
        delay(600);
        tone(Pins::BUZZER_D, 300, 500);
    }
}

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