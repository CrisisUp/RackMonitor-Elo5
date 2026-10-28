# 📋 RACK MONITOR — Sistema de Monitoramento de Rack com ESP32

[![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue?logo=espressif&logoColor=white)](https://www.espressif.com/)
[![MQTT](https://img.shields.io/badge/MQTT-broker.emqx.io-purple?logo=eclipse-mosquitto&logoColor=white)](https://mqtt.org/)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Ready-orange?logo=platformio&logoColor=white)](https://platformio.org/)
[![Tests](https://img.shields.io/badge/Tests-12%2F12-brightgreen)](./test/)
[![Doxygen](https://img.shields.io/badge/Docs-Doxygen-blue?logo=readthedocs&logoColor=white)](./docs/html/index.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/version-7.4-brightgreen)](./CHANGELOG.md)

Sistema IoT de monitoramento de rack de servidores com ESP32, sensores locais,
dashboard web em tempo real, controle remoto via MQTT, OTA (ArduinoOTA) e
Dashboard Local de contingência (SSE). **Totalmente modularizado e com 12 testes
nativos automatizados.**

Versão atual: **firmware v7.4**
Ambiente de desenvolvimento: **PlatformIO (VS Code) + Wokwi (protótipo) + MQTTX**

---

## 🎯 Visão Geral

Sistema IoT de monitoramento de rack de servidores com ESP32, sensores locais, dashboard web em tempo real, controle remoto via MQTT e Dashboard Local de contingência (SSE).

## 📚 Documentação Técnica Completa

A documentação detalhada de todas as classes, métodos, structs e variáveis foi gerada automaticamente com **Doxygen**.

👉 **[Clique aqui para acessar a documentação (HTML)](./docs/html/index.html)**

A documentação cobre:

- **Classes:** `Alarme`, `Audio`, `DisplayOLED`, `NotificadorMQTT`, `DashboardWeb`, `RingBufferLog`, `BotaoPanico`, etc.
- **Structs:** `LeiturasSensores`, `EventoRFID`, `EventoLog`
- **Enums:** `EstadoSistema`, `ModoVigilante`, `EstadoAmbiental`, `NivelLog`, `FonteTempo`
- **Funções:** `verificarAmbiente()`, `executarModoTeste()`, `montarJSONStatus()`, etc.

## 📦 Arquitetura Final

```text
┌─────────────────┐         ┌──────────────┐         ┌──────────────┐
│  ESP32          │  MQTT   │  Broker      │  WSS    │  Site Web    │
│  (hardware)     │ ──────► │  (emqx.io)   │ ◄─────► │  (Externo)   │
│                 │ ◄────── │              │         │              │
└─────────────────┘         └──────────────┘         └──────────────┘
     │                                                   │
     ├─ Sensores (PIR, LDR, US, Porta, RFID, DHT)        │
     ├─ Atuadores (LED, Buzzer, OLED)                    │
     ├─ Máquina de Estados (Alarme)                      │
     ├─ Dashboard Local (SSE na porta 8080)              │
     ├─ OTA (ArduinoOTA)                                 │
     └─ NVS (Persistência)                               │
```

## 🧩 Componentes do Sistema

### 1. Código do ESP32 (src/main.cpp)

O main.cpp é um orquestrador puro. Ele apenas inicializa os módulos no
setup() e coordena o ciclo no loop(). Toda a lógica está encapsulada em
bibliotecas na pasta lib/.

### 2. Arquitetura Modular (lib/)

| Biblioteca | Responsabilidade |
| :--- | :--- |
| **`alarme`** | Máquina de estados central (única fonte de verdade). |
| **`alarme_ambiental`** | Verificação de temperatura, umidade e taxa de subida. |
| **`audio`** | Controle dos buzzers com prioridade sonora. |
| **`botao`** | Botão multifunção (curto/duplo/longo) com debounce. |
| **`botao_panico`** | Botão de pânico com ISR (resposta < 30ms). |
| **`display_oled`** | Renderização do display OLED (SSD1306). |
| **`ldr`** | Sensor de luz com histerese e debounce. |
| **`led_rgb`** | LED de status RGB com PWM. |
| **`leitor_rfid`** | Leitor MFRC522 que emite eventos. |
| **`log_central`** | Log centralizado (Serial + MQTT + Ring Buffer). |
| **`notificador_mqtt`** | Publicação MQTT (status, ambiente, logs, heartbeat). |
| **`nvs`** | Persistência de config, contadores e cartões. |
| **`ota`** | Atualização remota via ArduinoOTA. |
| **`pir`** | Sensor de movimento. |
| **`porta`** | Switch com debounce. |
| **`relogio`** | Timestamp via RTC DS3231 (fallback NTP, depois millis). |
| **`sensor_dht`** | Leitura do DHT22 (temperatura e umidade). |
| **`serial_commands`** | Comandos Serial (SET, ARMAR, DESARMAR, HIST, etc.). |
| **`test_mode`** | Autodiagnóstico de hardware. |
| **`ultrassonico`** | HC-SR04 com média móvel de 5 amostras. |

### 3. Estrutura de Pastas Completa

```text
RackMonitor-Elo5/
├── include/
│   ├── arduino_stubs.h      # Mock do Arduino para testes nativos
│   ├── config.h             # Pinos, tempos, rede, MQTT, OTA
│   ├── json_builder.h       # Geração de JSON (reutilizado por MQTT e Dashboard)
│   ├── ring_buffer.h        # Buffer circular de 100 eventos
│   └── state.h              # Enums e structs compartilhados
├── lib/
│   ├── alarme/              # Máquina de estados central
│   ├── alarme_ambiental/    # Verificação de temperatura/umidade
│   ├── audio/               # Controle dos buzzers
│   ├── botao/               # Botão multifunção
│   ├── botao_panico/        # Botão de pânico com ISR
│   ├── display_oled/        # Display OLED
│   ├── ldr/                 # Sensor de luz
│   ├── led_rgb/             # LED RGB
│   ├── leitor_rfid/         # Leitor MFRC522
│   ├── log_central/         # Log centralizado
│   ├── notificador_mqtt/    # Publicação MQTT
│   ├── nvs/                 # Persistência NVS
│   ├── ota/                 # OTA (ArduinoOTA)
│   ├── pir/                 # Sensor de movimento
│   ├── porta/               # Switch da porta
│   ├── relogio/             # RTC + NTP
│   ├── sensor_dht/          # DHT22
│   ├── serial_commands/     # Comandos Serial
│   ├── test_mode/           # Autodiagnóstico
│   └── ultrassonico/        # HC-SR04
├── src/
│   ├── main.cpp             # Orquestrador (setup + loop)
│   └── ring_buffer.cpp      # Implementação do ring buffer
├── test/
│   └── test_logic/
│       ├── test_alarme.cpp
│       ├── test_json.cpp
│       ├── test_logic.cpp
│       └── test_ringbuffer.cpp
├── platformio.ini
├── README.md
├── CHANGELOG.md
├── TESTING.md
└── Doxyfile
```

### 4. Testes Nativos (PlatformIO + Unity)

O projeto possui **12 testes automatizados** que rodam no PC (ambiente `native`), sem necessidade de hardware. Eles cobrem:

- **Alarme:** 4 testes (prioridade de porta, RFID, movimento, caso normal).
- **JSON:** 2 testes (formato correto, tratamento de `null` e `-999`).
- **Ring Buffer:** 6 testes (vazio, push, conteúdo, overflow, clear, truncamento).

**Para rodar:**

```bash
pio test -e native
```

Saída esperada:

```text
12 test cases: 12 succeeded in 00:00:01.058
```

### 5. Compilação e Upload

```bash
# Compilar (sem hardware)
pio run -e esp32dev

# Upload para o ESP32
pio run -e esp32dev -t upload

# Monitor Serial
pio device monitor
```

### 6. Bibliotecas Necessárias (platformio.ini)

```ini
lib_deps = 
    miguelbalboa/MFRC522@^1.4.11
    adafruit/Adafruit GFX Library@^1.11.11
    adafruit/Adafruit SSD1306@^2.5.13
    adafruit/RTClib@^2.1.4
    adafruit/DHT sensor library@^1.4.6
    knolleary/PubSubClient@^2.8
```

## 🔧 Hardware

### Mapeamento de Pinos (ESP32 DevKit)

| GPIO | Módulo | Função | Observação |
| :---: | :--- | :--- | :--- |
| 0 | LedRGB | Canal B | Strapping (boot) |
| 2 | Audio | Buzzer D | Strapping (boot) |
| 4 | LeitorRFID | RST | |
| 5 | LeitorRFID | SS (SDA) | |
| 13 | DHT22 | Dados (SDA) | |
| 14 | Botao | Pushbutton | INPUT_PULLUP |
| 15 | Audio | Buzzer E | Strapping (boot) |
| 16 | Ultrassonico | ECHO | |
| 17 | Ultrassonico | TRIG | |
| 18 | LeitorRFID | SCK | |
| 19 | LeitorRFID | MISO | |
| 21 | DisplayOLED | I2C SDA | Wire.begin(21,22) |
| 22 | DisplayOLED | I2C SCL | Wire.begin(21,22) |
| 23 | LeitorRFID | MOSI | |
| 25 | Porta | Switch | INPUT_PULLDOWN |
| 26 | BotaoPanico | Pushbutton | INPUT_PULLUP + ISR |
| 27 | PIR | Movimento | |
| 32 | LedRGB | Canal R | |
| 33 | LedRGB | Canal G | |
| 34 | LDR | Leitura Analógica | Input-only |

## 🧠 Regras de Ouro (Arquitetura)

1. **Sensores não decidem** — só medem e filtram.
2. **Atuadores não leem sensores** — só consomem estado.
3. **Alarme é fonte única de verdade** — recebe struct/eventos.
4. **Eventos são fatos** — botão foi pressionado, cartão foi lido.
5. **Toda transição é logada** — auditoria completa.
6. **`loop()` só orquestra** — update → coleta eventos → renderiza.
7. **MQTT é observador** — comandos remotos pedem via API pública.
8. **NVS é passiva** — não interfere na lógica do alarme.
9. **JSON é montado uma vez** — `montarJSONStatus()` reutilizado por MQTT e Dashboard.
10. **Logs alimentam o ring buffer** — toda chamada a `logSerial()` é auditável.

## 📡 Comunicação MQTT (v7.4)

| Tópico | Direção | Retain | Conteúdo |
| :--- | :--- | :---: | :--- |
| `rack_monitor_ABCD1234/status` | ESP32 → Site | ✅ | JSON com estado, modo, sensores, contadores, flags ambientais |
| `rack_monitor_ABCD1234/status/lwt` | Broker → Site | ✅ | `{"status":"online"}` ou `{"status":"offline"}` (LWT) |
| `rack_monitor_ABCD1234/comando` | Site → ESP32 | ❌ | `ARMAR`, `DESARMAR`, `STATUS`, `RESET <token>`, `TESTE` |
| `rack_monitor_ABCD1234/ambiente` | ESP32 → Site | ✅ | JSON dedicado: temp, umid, estado ambiental, `sensor_dht_ok`, limiares |
| `rack_monitor_ABCD1234/comando/ambiente` | Site → ESP32 | ❌ | `SET_TEMP_ALERTA`, `SET_TEMP_CRIT`, `PANICO`, `STATUS_AMB` |
| `rack_monitor_ABCD1234/logs` | ESP32 → Site | ❌ | Eventos detalhados (porta, cartão, transições, ambiente) |
| `rack_monitor_ABCD1234/heartbeat` | ESP32 → Site | ❌ | Sinal de vida a cada 30 s |

## 💾 Persistência NVS

Salvos na flash do ESP32:

- **Configuração:** `limiar`, `timeout`, `t_alert`, `t_crit`, `u_alert`, `u_crit`.
- **Contadores:** `invasoes`, `aberturas`, `acessos`, `desarmes`, `rearmes`.
- **Cartões RFID:** `total_cards` + `card_0`, `card_1`, …
- **Modo:** `modo` (ARMADO / DESARMADO — persistido entre reboots e OTA).

## 🖥️ Comandos Serial (firmware v7.4)

| Comando | Descrição |
| :--- | :--- |
| `SET LIMIAR <5-400>` | Ajusta limiar ultrassônico (persiste) |
| `SET TIMEOUT <0-1440>` | Timeout de desarme em minutos (persiste) |
| `SET TEMP_ALERTA <15-50>` | Limiar de temperatura para alerta (persiste) |
| `SET TEMP_CRIT <20-60>` | Limiar de temperatura crítico (persiste) |
| `SET UMID_ALERTA <30-90>` | Limiar de umidade para alerta (persiste) |
| `SET UMID_CRIT <40-100>` | Limiar de umidade crítico (persiste) |
| `STATUS` | Status completo (contadores, limiares, Wi-Fi, MQTT, OTA, histórico) |
| `ARMAR` | Força `ARMADO` (se porta fechada) |
| `DESARMAR` | Força `DESARMADO` |
| `TESTE` | Modo autodiagnóstico |
| `PANICO` | Dispara pânico manual |
| `RTC` | Mostra hora e fonte de tempo |
| `RTC YYYY-MM-DD HH:MM:SS` | Ajusta RTC manualmente |
| `NTP` | Força sincronização NTP manual |
| `OTA` | Mostra status do serviço OTA |
| `HIST` | Despeja os últimos 100 eventos do ring buffer |
| `HIST_JSON` | Despeja o histórico em formato JSON |
| `HIST_CLEAR` | Limpa o histórico de eventos |
| `RESET` | Zera contadores operacionais (preserva segurança) |
| `ADD_CARD <UID>` | Cadastra cartão na NVS (deduplicado) |
| `LIST_CARDS` | Lista cartões autorizados |
| `HELP` | Lista comandos |

### Botão físico (multifunção)

- **Curto** — reset operacional
- **Longo** — rearme forçado
- **Duplo** — modo teste

### Botão de pânico (GPIO 26)

- **ISR** (FALLING edge) — resposta em < 30 ms
- **Cooldown** de 2 s (evita bounce mecânico)
- Dispara `INVASAO_SALA` imediatamente
- Ignorado em modo `DESARMADO`

---

## 🧪 Testes

```bash
# Testes nativos (rápidos, sem hardware)
pio test -e native

# Compilação do ESP32
pio run -e esp32dev
```

## 🎯 Princípios de Arquitetura Aplicados

1. **Sensores não decidem** — só medem e filtram.
2. **Atuadores não leem sensores** — só consomem estado.
3. **Alarme é fonte única de verdade** — recebe struct/eventos.
4. **Eventos são fatos** — botão foi pressionado, cartão foi lido.
5. **Toda transição é logada** — auditoria completa.
6. **`loop()` só orquestra** — update → coleta eventos → renderiza.
7. **MQTT é observador** — comandos remotos pedem via API pública.
8. **NVS é passiva** — não interfere na lógica do alarme.
9. **JSON é montado uma vez** — `montarJSONStatus()` reutilizado por MQTT e Dashboard.
10. **Logs alimentam o ring buffer** — toda chamada a `logSerial()` é auditável.

## ✅ O que está funcionando (v7.4)

### Firmware

- ☑ Sensores com filtros e debounce
- ☑ Máquina de estados com transições
- ☑ RFID + botão emitindo eventos
- ☑ Vigilante com bloqueio de rearme
- ☑ Wi-Fi + MQTT (LWT, Retain, comandos segregados)
- ☑ NVS (persistência de config, contadores, limiares, cartões, modo)
- ☑ Logs de auditoria via MQTT
- ☑ Alarmes ambientais (temp/umid) com histerese, crítico e taxa de subida
- ☑ Estado ambiental consolidado no JSON
- ☑ Rate limit de `STATUS` (10 s) + `RESET` com token
- ☑ Dirty flag para reduzir desgaste da flash
- ☑ **NTP + RTC Sync** com fallback para millis
- ☑ **OTA (ArduinoOTA)** com callbacks de progresso
- ☑ **Botão de pânico** com ISR (< 30 ms)
- ☑ **Ring Buffer** de 100 eventos em RAM
- ☑ **Dashboard Local (SSE)** na porta 8080
- ☑ **Persistência do modo** entre reboots e OTA

### Arquitetura

- ☑ **20 bibliotecas** na pasta `lib/`
- ☑ `main.cpp` como orquestrador puro (~280 linhas)
- ☑ **12 testes nativos** passando (alarme, JSON, ring buffer)
- ☑ Documentação embutida (Doxygen) e CHANGELOG

---

## 🚀 Próximos passos sugeridos

- **HMAC nos comandos MQTT** — assinar `ARMAR`/`DESARMAR` com SHA256 + timestamp.
- **Broker privado** — migrar do `broker.emqx.io` para um HiveMQ Cloud com TLS.
- **CI/CD (GitHub Actions)** — rodar os 12 testes automaticamente a cada push.
- **Testes E2E** — validar o sistema completo via MQTT com Python.
- **Testes no hardware físico** — validar OTA, NTP e Dashboard Local.
- **Notificação push** — integrar com Telegram ou ntfy.sh.
- **Persistência do histórico** — backend com InfluxDB / SQLite.

---

## 📌 Lembretes importantes

- **Backup:** baixe uma cópia do `src/main.cpp` e `index.html`.
- **Bibliotecas:** devem estar no `platformio.ini`.
- **`broker.emqx.io`** — broker público gratuito.
- **Porta 8084 (WSS)** — usada pelo navegador; **1883 (TCP)** — usada pelo ESP32.
- **Porta 8080 (HTTP)** — usada pelo Dashboard Local no hardware físico.
- **Prefixos de tópico** — o `rack_monitor_ABCD1234` deve ser idêntico no firmware e no dashboard.
- **Token de RESET** — troque `rack2026` por um valor seu no `config.h` antes de produção.
- **Wi-Fi** — troque `Wokwi-GUEST` pelas credenciais da sua rede real no `config.h`.

## 📚 Referências e Agradecimentos

Este projeto foi construído sobre os ombros de gigantes. Agradecimentos às seguintes documentações e ferramentas:

### Bibliotecas e Ferramentas

- [**PlatformIO**](https://platformio.org/) — IDE e build system para embarcados
- [**Wokwi**](https://docs.wokwi.com/) — Simulador de ESP32 (gratuito)
- [**PubSubClient**](https://pubsubclient.knolleary.net/) — Cliente MQTT
- [**MFRC522**](https://github.com/miguelbalboa/rfid) — Biblioteca do RFID
- [**Adafruit SSD1306**](https://learn.adafruit.com/monochrome-oled-breakouts) + [**GFX**](https://learn.adafruit.com/adafruit-gfx-graphics-library) — Display OLED
- [**RTClib**](https://learn.adafruit.com/adafruit-ds3231-precision-rtc-breakout) — Driver do RTC DS3231
- [**DHT sensor library**](https://learn.adafruit.com/dht) — Sensor DHT22
- [**Preferences**](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html) — NVS
- [**ArduinoOTA**](https://arduino-esp32.embarkx.in/.../ArduinoOTA) — OTA
- [**WebServer**](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer) — Servidor HTTP built-in
- [**Doxygen**](https://www.doxygen.nl/) — Documentação automática

### Conceitos e Tutoriais

- [MQTT Essentials (HiveMQ)](https://www.hivemq.com/mqtt-essentials/) — Introdução ao MQTT
- [MQTT Retain and LWT (EMQX)](https://www.emqx.com/en/blog/mqtt-retained-messages) — Retain e Last Will
- [ESP32 Strapping Pins (Espressif)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html) — Cuidados com GPIOs 0, 2, 12 e 15
- [Server-Sent Events (MDN)](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events) — Referência do SSE
- [ArduinoOTA (ESP32)](https://arduino-esp32.embarkx.in/...) — Como funciona o OTA no ESP32

### Inspiração

- [Arduino Reference](https://www.arduino.cc/reference/en/) — Boas práticas de código embarcado
- [The Twelve-Factor App](https://12factor.net/) — Princípios de design de software

```text

---

Este é o **`README.md` completo e atualizado para a v7.4**. Ele reflete a nova estrutura modular, os 12 testes nativos, a pasta `lib/` com 20 bibliotecas, e mantém toda a documentação e badges profissionais. Você pode substituir o conteúdo do seu `README.md` atual por este bloco inteiro.
```

## 📌 Próximos passos (quando quiser continuar)

O projeto está pronto para produção, mas sempre há espaço para evoluir:

- **HMAC nos comandos MQTT** — assinar `ARMAR`/`DESARMAR` com SHA256.
- **CI/CD (GitHub Actions)** — rodar os 12 testes automaticamente.
- **Testes E2E** — validar o sistema completo via MQTT com Python.
- **Testes no hardware físico** — validar OTA, NTP e Dashboard Local.
- **Broker privado** — migrar do `broker.emqx.io` para HiveMQ Cloud.
- **Notificação push** — integrar com Telegram ou ntfy.sh.
- **Persistência do histórico** — backend com InfluxDB ou SQLite.
