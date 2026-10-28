#!/usr/bin/env bash
# =========================================================================
# rebuild_and_capture_key.sh
#
# Automatiza:
#   1. Lembrete para limpar o localStorage do navegador
#   2. Erase da NVS do ESP32
#   3. Upload do firmware
#   4. Abertura do monitor serial para capturar a chave HMAC
#
# Uso:
#   chmod +x rebuild_and_capture_key.sh
#   ./rebuild_and_capture_key.sh
# =========================================================================

set -e  # aborta em qualquer erro

# --- Configurações ---
ENV="esp32dev"
PORT="/dev/cu.usbserial-0001"
BAUD="115200"
MONITOR_BAUD="115200"

# Cores (se o terminal suportar)
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${YELLOW}=================================================${NC}"
echo -e "${YELLOW}  1. LIMPAR localStorage DO NAVEGADOR${NC}"
echo -e "${YELLOW}=================================================${NC}"
echo ""
echo "Abra o dashboard no navegador e execute no console (F12 → Console):"
echo ""
echo -e "   ${GREEN}localStorage.removeItem('hmac_key');${NC}"
echo -e "   ${GREEN}location.reload();${NC}"
echo ""
read -p "Pressione ENTER quando tiver feito isso..."

# --- 2. Erase da NVS ---
echo -e "${YELLOW}=================================================${NC}"
echo -e "${YELLOW}  2. ERASE DA NVS (flash do ESP32)${NC}"
echo -e "${YELLOW}=================================================${NC}"
pio run -e "$ENV" -t erase || {
    echo -e "${RED}Erro no erase. Tentando via esptool direto...${NC}"
    ~/.platformio/packages/tool-esptoolpy/esptool.py \
        --chip esp32 --port "$PORT" --baud "$BAUD" erase_flash
}

# --- 3. Upload do firmware ---
echo -e "${YELLOW}=================================================${NC}"
echo -e "${YELLOW}  3. UPLOAD DO FIRMWARE${NC}"
echo -e "${YELLOW}=================================================${NC}"
pio run -e "$ENV" -t upload || {
    echo -e "${RED}Erro no upload. Tentando via esptool direto (--no-stub)...${NC}"
    ~/.platformio/packages/tool-esptoolpy/esptool.py \
        --chip esp32 --port "$PORT" --baud "$BAUD" --no-stub \
        write_flash 0x10000 .pio/build/$ENV/firmware.bin
}

# --- 4. Monitor serial ---
echo -e "${YELLOW}=================================================${NC}"
echo -e "${YELLOW}  4. MONITOR SERIAL (Ctrl+C para sair)${NC}"
echo -e "${YELLOW}=================================================${NC}"
echo ""
echo "Aguardando a chave HMAC no boot. Procure por:"
echo "  [HMAC] Chave gerada e salva na NVS:"
echo ""
pio device monitor --baud "$MONITOR_BAUD"

# --- 5. Instruções finais ---
echo ""
echo -e "${GREEN}=================================================${NC}"
echo -e "${GREEN}  PRÓXIMOS PASSOS${NC}"
echo -e "${GREEN}=================================================${NC}"
echo ""
echo "1. Copie a chave que apareceu no monitor (linha após 'Chave gerada...')."
echo "2. Cole no campo amarelo do dashboard e clique em Salvar."
echo "3. Cadastre o cartão no monitor serial (digite e Enter):"
echo ""
echo -e "   ${GREEN}ADD_CARD D4A33F02${NC}"
echo ""
echo "4. Aproxime o cartão do PN532 para testar."
echo ""