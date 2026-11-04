#!/bin/bash
# run_tests.sh
echo "🧪 Rodando testes unitários no ambiente nativo..."
docker run --rm -v "$(pwd)":/app -w /app esp32-seguranca pio test -e native