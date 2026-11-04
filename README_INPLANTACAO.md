# RackMonitor-Elo5

Sistema de monitoramento e segurança para racks, desenvolvido com ESP32.
O projeto inclui funcionalidades de alarme, leitura de sensores (RFID, PIR, LDR, DHT, ultrassônico), notificações via MQTT e interface web local.

## 📋 Índice

- [RackMonitor-Elo5](#rackmonitor-elo5)
  - [📋 Índice](#-índice)
  - [✨ Funcionalidades](#-funcionalidades)
  - [🔌 Hardware Utilizado](#-hardware-utilizado)
  - [🏗️ Arquitetura do Projeto](#️-arquitetura-do-projeto)
  - [🐳 Como Compilar e Testar (Docker)](#-como-compilar-e-testar-docker)
  - [📖 Guia completo: Para instruções detalhadas sobre Docker (incluindo solução de problemas de porta USB no macOS), consulte o README\_DOCKER.md](#-guia-completo-para-instruções-detalhadas-sobre-docker-incluindo-solução-de-problemas-de-porta-usb-no-macos-consulte-o-readme_dockermd)
  - [🔌 Como Gravar no ESP32](#-como-gravar-no-esp32)
  - [🔐 Segurança e Configuração](#-segurança-e-configuração)
  - [📚 Documentação Adicional](#-documentação-adicional)
  - [📄 Licença](#-licença)
    - [💡 O que esse README faz?](#-o-que-esse-readme-faz)

---

## ✨ Funcionalidades

- **Alarme de Segurança:** Detecção de invasão via sensor de porta (Reed Switch), PIR e botão de pânico.
- **Controle de Acesso:** Leitor RFID (PN532) com autenticação de UIDs.
- **Monitoramento Ambiental:** Sensores de temperatura/umidade (DHT) e luminosidade (LDR) com limites configuráveis.
- **Interface Web Local:** Dashboard acessível via Wi-Fi (pasta `server/`).
- **Comunicação MQTT:** Envio de status, heartbeats e logs para um broker (ex: EMQX, Mosquitto).
- **OTA (Over-The-Air):** Atualização de firmware sem cabo USB.
- **Criptografia HMAC:** Comandos críticos (armar/desarmar) validados com assinatura HMAC para evitar falsificações.

## 🔌 Hardware Utilizado

- **Microcontrolador:** ESP32 (Dev Module).
- **Sensores/Atuadores:**
  - Leitor RFID PN532 (SPI ou I2C).
  - Display OLED SSD1306 (I2C).
  - Sensor de temperatura/umidade DHT11/DHT22.
  - Sensor PIR (movimento).
  - Sensor ultrassônico HC-SR04.
  - LDR (luminosidade).
  - Buzzer, botões e LEDs RGB.

> **Nota:** O mapeamento de pinos está definido em `include/config.h` (ou `config.example.h`).

## 🏗️ Arquitetura do Projeto

O projeto segue a estrutura modular do **PlatformIO**, separando o código em **bibliotecas independentes** para facilitar a manutenção e os testes.

```text
RackMonitor-Elo5/
├── include/           # Configurações globais e mapeamento de hardware
├── lib/               # Módulos do projeto (alarme, display, rfid, etc.)
├── src/               # Código principal (main.cpp)
├── test/              # Testes unitários (rodam no PC)
├── server/            # Arquivos da interface web
├── tools/             # Scripts auxiliares (ex: assinatura HMAC)
├── Dockerfile         # Ambiente de build isolado
└── platformio.ini     # Configuração de ambientes (esp32dev, native)
```

## 🐳 Como Compilar e Testar (Docker)

O projeto utiliza Docker para garantir que o ambiente de compilação seja idêntico em qualquer máquina. Você não precisa ter o PlatformIO instalado localmente.

Construa a imagem de ferramentas (só é necessário na primeira vez ou se mudar o platformio.ini):

```bash
docker build --target builder -t rackmonitor-tools .
```

Rode os testes unitários:

```bash
docker compose run --rm test
```

Compile o firmware:

```bash
docker compose run --rm build
```

## 📖 Guia completo: Para instruções detalhadas sobre Docker (incluindo solução de problemas de porta USB no macOS), consulte o README_DOCKER.md

## 🔌 Como Gravar no ESP32

Como o **Docker no macOS** tem limitações para acessar a porta USB diretamente, a forma mais prática de gravar é usar o **PlatformIO nativo** (VS Code):

1. Abra o projeto no **VS Code** (com a extensão PlatformIO instalada).
2. Conecte o **ESP32** via USB.
3. Clique no ícone de **Upload** (seta para a direita) na barra inferior.

> **Alternativa:** Se você estiver em Linux ou quiser usar o Docker para upload, siga as instruções do **[README_DOCKER.md](README_DOCKER.md)**.

## 🔐 Segurança e Configuração

O arquivo include/config.h contém senhas de Wi-Fi, MQTT, OTA e a chave HMAC. Ele está no .gitignore e nunca deve ser commitado.

Copie o arquivo de exemplo:

```bash
cp include/config.example.h include/config.h
```

Edite o include/config.h e preencha com os seus dados reais (SSID, senha, broker, etc.).

## 📚 Documentação Adicional

- **[README_DOCKER.md](README_DOCKER.md):** Guia completo de uso do Docker, incluindo solução de problemas.
- **[SIMBOLOS_README.md](SIMBOLOS_README.md):** Legenda de símbolos e diagramas do projeto (se aplicável).

## 📄 Licença

Este projeto é de uso pessoal e educacional. Sinta-se livre para estudar, modificar e adaptar o código. Se for redistribuir, dê os créditos apropriados.

---

### 💡 O que esse README faz?

1. **Apresenta o projeto:** Explica o que é e para que serve.
2. **Organiza a leitura:** Um índice clicável ajuda a navegar.
3. **Lista o hardware:** Facilita a reprodução por terceiros.
4. **Mostra a arquitetura:** Ajuda quem vai mexer no código a se localizar.
5. **Direciona para o Docker:** Como o ambiente principal é o Docker, o README principal aponta para o `README_DOCKER.md`.
6. **Explica como gravar:** Como o upload via Docker no Mac é problemático, ele sugere a alternativa nativa.
7. **Alerta sobre segurança:** Lembra de configurar o `config.h` e não commitá-lo.

Basta salvar esse conteúdo no `README.md` da raiz do projeto e o repositório estará com uma cara muito mais profissional! 🚀
