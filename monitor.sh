#!/bin/bash
# monitor.sh

PORTA=$(ls /dev/cu.usbserial-* 2>/dev/null | head -n 1)

if [ -z "$PORTA" ]; then
    echo "❌ Nenhuma porta serial encontrada."
    exit 1
fi

docker run --rm -it \
    --device="$PORTA" \
    -v "$(pwd)":/app \
    -w /app \
    esp32-seguranca \
    pio device monitor -e esp32dev