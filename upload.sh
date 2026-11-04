#!/bin/bash
# upload.sh

# Descobre a porta serial automaticamente (macOS)
PORTA=$(ls /dev/cu.usbserial-* 2>/dev/null | head -n 1)

if [ -z "$PORTA" ]; then
    echo "❌ Nenhuma porta serial encontrada. Conecte o ESP32."
    exit 1
fi

echo "🔌 Usando a porta: $PORTA"
echo "🚀 Compilando e gravando o firmware..."

docker run --rm -it \
    --device="$PORTA" \
    -v "$(pwd)":/app \
    -w /app \
    esp32-seguranca \
    pio run -e esp32dev -t upload