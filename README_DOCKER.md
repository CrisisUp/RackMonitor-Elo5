# 🐳 Guia de Uso com Docker

Este documento explica como utilizar o Docker para compilar, testar e gravar o firmware do projeto **ESP32-Segurança** em qualquer sistema operacional, sem precisar instalar o PlatformIO ou o toolchain do ESP32 na máquina local.

## 📋 Pré-requisitos

* **Docker** instalado e em execução.
  * [Docker Desktop para macOS](https://docs.docker.com/desktop/install/mac-install/)
  * [Docker Engine para Linux](https://docs.docker.com/engine/install/)
* **Docker Compose** (já incluso no Docker Desktop e na maioria das instalações Linux).

> **⚠️ Importante:** No macOS e Windows, o Docker roda em uma máquina virtual e não tem acesso direto à porta USB. Para gravar o firmware, você precisa mapear o dispositivo (veja a seção [Gravação no ESP32](#-3-gravação-no-esp32)).

## 📁 Estrutura de Arquivos

Os arquivos relacionados ao Docker são:

```text
esp32-seguranca/
├── Dockerfile              # Receita da imagem (multi-stage)
├── docker-compose.yml      # Orquestração dos serviços (test, build, upload, monitor)
├── .dockerignore           # Arquivos ignorados no contexto de build
├── upload.sh               # Script para gravar o firmware (detecta a porta no macOS)
├── monitor.sh              # Script para abrir o monitor serial
├── run_tests.sh            # Script para rodar os testes unitários
└── README_DOCKER.md        # Este arquivo
```

## 🚀 Construindo a Imagem

Antes de usar qualquer serviço, você precisa construir a imagem Docker. A imagem usa **multi-stage build**: o primeiro estágio compila o firmware, e o segundo estágio cria uma imagem leve com apenas os binários.

```bash
docker compose build
```

> **💡 Dica:** Se você alterar apenas o código `.cpp` ou `.h`, o Docker aproveita o cache e a compilação é rápida. Se alterar o `platformio.ini` ou adicionar novas dependências, ele baixa tudo de novo.
>
> **⏱️ Tempo de build:**
>
> * **Primeira build:** ~5 a 10 minutos (baixa o toolchain do ESP32 e todas as bibliotecas).
> * **Builds subsequentes (sem mudanças):** ~5 segundos (tudo em cache).
> * **Builds com mudanças no código:** ~1 minuto (recompila o firmware).
> * **Builds com mudanças no `platformio.ini`:** ~5 minutos (baixa novas dependências).

## 🧪 1. Rodar os Testes Unitários

Os testes rodam no ambiente **`native`** (no PC), sem precisar do ESP32. Eles garantem que a lógica do código está correta antes de compilar para o hardware.

```bash
docker compose run --rm test
```

**O que esse comando faz:**

* Usa o serviço `test` definido no `docker-compose.yml`.
* Executa `pio test -e native` dentro do container.
* Mostra no terminal os resultados dos testes (passou/falhou).

## 🛠️ 2. Compilar o Firmware

Para compilar o firmware sem gravar no ESP32 (útil para verificar se o código compila):

```bash
docker compose run --rm build
```

O binário gerado estará em .pio/build/esp32dev/firmware.bin na sua máquina local (por causa do volume .:/app).

## 🔌 3. Gravação no ESP32

A gravação exige que o container tenha acesso à porta serial. No **macOS**, a porta serial geralmente é algo como `/dev/cu.usbserial-0001`. No **Linux**, é `/dev/ttyUSB0` ou `/dev/ttyACM0`.

### macOS

Use o script `upload.sh`, que detecta a porta automaticamente:

```bash
chmod +x upload.sh
./upload.sh
```

Ou, se preferir o docker-compose.yml, defina a variável de ambiente e rode:

```bash
export ESP32_PORT=$(ls /dev/cu.usbserial-* 2>/dev/null | head -n 1)
docker compose run --rm upload
Linux
bash
# Descubra a porta
ls /dev/ttyUSB*
# Exemplo: /dev/ttyUSB0

# Defina a variável e rode
export ESP32_PORT=/dev/ttyUSB0
docker compose run --rm upload
```

⚠️ Permissões no Linux: Se você receber um erro de "permissão negada", adicione seu usuário ao grupo dialout:

```bash
sudo usermod -a -G dialout $USER
```

Depois, faça logout e login novamente.

### Windows

O Docker no Windows não tem acesso direto às portas COM. Você precisa usar o **WSL 2** com suporte a USB (via `usbipd`) ou rodar o comando `pio run -e esp32dev -t upload` diretamente no PlatformIO instalado no Windows.

## 📺 4. Monitor Serial

Para ver os logs do ESP32 em tempo real:

```bash
chmod +x monitor.sh
./monitor.sh
```

Ou com o compose:

```bash
docker compose run --rm monitor
```

> **💡 Dica:** O monitor serial do PlatformIO no Docker pode não exibir cores ANSI. Se os logs parecerem estranhos, pressione `Ctrl+C` e rode novamente.

## 🔐 Variáveis de Ambiente e o config.h

O seu projeto usa um `include/config.h` com senhas reais (Wi-Fi, MQTT, OTA). Esse arquivo **está no `.dockerignore`** para não ir para a imagem Docker.

Durante a build, o Docker usa o `include/config.example.h` como base. Isso significa que:

* Os testes rodam com valores fictícios (o que é seguro).
* A compilação funciona, mas o firmware gerado **não terá suas senhas reais**.

Para usar as senhas reais, você tem duas opções:

1. **Montar o `config.h` como volume** (recomendado para testes):

```bash
docker compose run --rm -v "$(pwd)/include/config.h:/app/include/config.h" build
```

Criar uma imagem personalizada com suas senhas (não recomendado por segurança).

### 🧹 Limpeza e Manutenção

Remover os containers e imagens do projeto

```bash
docker compose down --rmi all --volumes
```

### Limpar o cache de build do Docker

Se você precisar reconstruir a imagem do zero (sem cache):

```bash
docker compose build --no-cache
```

## 🆘 Solução de Problemas

### 1. "Cannot connect to the Docker daemon"

O Docker não está em execução. Inicie o Docker Desktop (macOS/Windows) ou o serviço (sudo systemctl start docker no Linux).

### 2. "Permission denied" ao acessar a porta serial (Linux)

Adicione seu usuário ao grupo dialout:

```bash
sudo usermod -a -G dialout $USER
```

Faça logout e login novamente.

### 3. "No such file or directory" ao montar /dev/cu.usbserial-* (macOS)

A porta serial pode ter outro nome (ex: `/dev/cu.SLAB_USBtoUART`). Descubra com:

```bash
ls /dev/cu.*
```

E defina ESP32_PORT manualmente.

### 4. A imagem Docker está muito grande

Verifique se o `.dockerignore` está funcionando. O comando `docker build` mostra o tamanho do contexto no início:

```text
Sending build context to Docker daemon  2.5MB
Se for maior que 100MB, o .pio/ não está sendo ignorado.
```

### 5. Erro de compilação por falta do config.h

Se você receber um erro como `fatal error: config.h: No such file or directory`, significa que o Docker não encontrou o `config.example.h`. Confirme se o arquivo está na pasta `include/` e se o Dockerfile tem a linha:

```dockerfile
COPY include/config.example.h ./include/config.h
```

## 📚 Referências

* [Documentação oficial do PlatformIO](https://docs.platformio.org/)
* [Docker Compose file reference](https://docs.docker.com/compose/compose-file/)
* [Guia de dispositivos USB no Docker](https://docs.docker.com/desktop/features/usb/)

## 📝 Notas Finais

Este ambiente Docker foi projetado para ser **reprodutível**: qualquer pessoa com Docker instalado pode compilar e testar o projeto sem precisar configurar o PlatformIO manualmente. Se você encontrar problemas, abra uma *issue* ou entre em contato.

text

---

### 💡 O que esse README cobre?

1. **Pré-requisitos:** O que instalar antes de começar.
2. **Estrutura:** Onde cada arquivo Docker está.
3. **Build:** Como construir a imagem.
4. **Testes:** Como rodar `pio test` no Docker.
5. **Compilação:** Como gerar o binário.
6. **Gravação:** Comandos específicos para macOS, Linux e Windows.
7. **Monitor Serial:** Como ver os logs.
8. **Variáveis de Ambiente:** Como lidar com o `config.h` sensível.
9. **Limpeza:** Como remover containers e imagens.
10. **Troubleshooting:** Soluções para os erros mais comuns.

Esse README é o complemento ideal para o seu projeto, deodand claro que o Docker é uma ferramenta de **desenvolvimento e build**, não apenas um detalhe técnico. 🚀
