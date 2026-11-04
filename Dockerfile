# =========================================
# STAGE 1: Build (Compilação e Testes)
# =========================================
FROM python:3.11-slim AS builder

# Evita que o Python escreva arquivos .pyc e permite logs em tempo real
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PLATFORMIO_CORE_DIR=/opt/platformio

# Instala dependências do sistema necessárias para o PlatformIO e toolchain
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Instala o PlatformIO Core
RUN pip install --no-cache-dir platformio

# Define o diretório de trabalho
WORKDIR /app

# Copia os arquivos do projeto (o .dockerignore vai ignorar o que não presta)
COPY . .

# --- CORREÇÃO 1: Cria o config.h a partir do exemplo ---
COPY include/config.example.h ./include/config.h

# 1. Roda os testes unitários no ambiente nativo (PC)
RUN pio test -e native

# 2. Compila o firmware para o ESP32
RUN pio run -e esp32dev

# =========================================
# STAGE 2: Runtime (Imagem final)
# =========================================
FROM python:3.11-slim AS runtime

LABEL maintainer="Seu Nome"
LABEL description="Ambiente para compilar e gravar firmware ESP32 com PlatformIO"

# Instala dependências mínimas de runtime (apenas o essencial)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    && rm -rf /var/lib/apt/lists/*

# Instala o PlatformIO novamente (é necessário para o comando pio)
RUN pip install --no-cache-dir platformio

# Cria um usuário não-root para segurança
RUN useradd -m -u 1000 pio

# Define o diretório de trabalho
WORKDIR /app

# Copia o binário compilado do estágio anterior
COPY --from=builder /app/.pio/build/esp32dev/firmware.bin ./firmware.bin

# --- CORREÇÃO 2: Copia o projeto completo para permitir upload/monitor ---
COPY --from=builder /app/platformio.ini ./
COPY --from=builder /app/src ./src
COPY --from=builder /app/lib ./lib
COPY --from=builder /app/include ./include
COPY --from=builder /app/test ./test

# Muda para o usuário não-root
USER pio

# Comando padrão: abre o bash
CMD ["bash"]