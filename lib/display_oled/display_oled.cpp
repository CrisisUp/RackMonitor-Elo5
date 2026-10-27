#include "display_oled.h"
#include "../../include/ring_buffer.h" // Se precisar de algo do buffer

// Inicializa o display no construtor
DisplayOLED::DisplayOLED() : _display(LARGURA_TELA, ALTURA_TELA, &Wire, -1) {}

void DisplayOLED::begin()
{
    if (!_display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED))
    {
        Serial.println(F("[ERRO] Falha ao inicializar SSD1306"));
    }
    _display.clearDisplay();
    _display.display();
}

void DisplayOLED::mostrarStatus(EstadoSistema estado, ModoVigilante modo,
                                const LeiturasSensores &s,
                                const String &uidDesarme, bool cartaoOk,
                                unsigned int contAberturas, unsigned int contInvasoes,
                                const String &uidRearme)
{
    _display.clearDisplay();
    String linha1, linha2;
    int icone = 1;

    switch (estado)
    {
    case SEGURO:
        linha1 = "Vigilante Armado";
        linha2 = "Porta Fechada";
        icone = 1;
        break;
    case ACESSO_RFID:
        linha1 = cartaoOk ? "Cartao Autorizado" : "Cartao Negado";
        linha2 = "UID: " + uidDesarme;
        icone = cartaoOk ? 1 : 3;
        break;
    case AVISO_SALA:
        linha1 = "Aviso: Movimento";
        linha2 = "Rack Fechado";
        icone = 2;
        break;
    case APROXIMACAO_RACK:
        linha1 = "Aproximacao";
        if (s.distancia < 0.0f)
            linha2 = "Dist: N/A";
        else
            linha2 = "Dist: " + String(s.distancia, 1) + "cm";
        icone = 2;
        break;
    case INVASAO_SALA:
        linha1 = "Invasao na Sala";
        linha2 = "Mov+Luz | #" + String(contInvasoes);
        icone = 3;
        break;
    case EMERGENCIA_RACK:
        linha1 = "Emergencia: Rack";
        linha2 = "Porta Aberta #" + String(contAberturas);
        icone = 3;
        break;
    }

    _desenharIcone(icone, 2, 2);
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(24, 4);
    _display.println(linha1);
    _display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    _display.setCursor(0, 26);
    _display.println(linha2);
    _display.drawLine(0, 46, 128, 46, SSD1306_WHITE);

    _display.setCursor(0, 52);
    _display.print(relogio.timestamp().substring(6));
    _display.print(" ");
    if (estadoAmbiental == AMB_CRITICO)
        _display.print("[!]");
    else if (estadoAmbiental == AMB_ALERTA)
        _display.print("[*]");
    else
        _display.print("[ ]");

    if (_ultimaTempValida > -100.0f)
    {
        _display.print(String(_ultimaTempValida, 0));
        _display.print("C/");
        _display.print(String(_ultimaUmidValida, 0));
        _display.print("%");
    }
    else
    {
        _display.print("--C/--%");
    }

    _display.setCursor(0, 60);
    _display.print("UID:");
    String uidExibir = (modo == ARMADO)
                           ? (uidRearme.length() > 0 ? uidRearme : "-")
                           : uidDesarme;
    _display.print(uidExibir);

    _display.display();
}

// ... (As funções mostrarDesarmado, mostrarBloqueio e _desenharIcone seguem o mesmo padrão)
// Vou colocar o resto abaixo para não cortar.

void DisplayOLED::mostrarDesarmado(unsigned long restanteMs, const String &uid,
                                   unsigned int movManutencao)
{
    _display.clearDisplay();
    _desenharIcone(4, 2, 2);
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(24, 4);
    _display.println("MANUTENCAO");
    _display.drawLine(0, 20, 128, 20, SSD1306_WHITE);

    unsigned long min = restanteMs / 60000UL;
    unsigned long seg = (restanteMs % 60000UL) / 1000UL;

    _display.setCursor(0, 26);
    _display.print("Rearme em: ");
    _display.print(min);
    _display.print("m");
    _display.print(seg);
    _display.print("s");

    _display.setCursor(0, 36);
    _display.print("UID: ");
    _display.print(uid);

    _display.drawLine(0, 46, 128, 46, SSD1306_WHITE);
    _display.setCursor(0, 52);
    _display.print(relogio.timestamp());
    _display.print("  MANUT:");
    _display.print(movManutencao);
    _display.display();
}

void DisplayOLED::mostrarBloqueio(const String &motivo, unsigned int totalBloqueios,
                                  const String &uid)
{
    _display.clearDisplay();
    _desenharIcone(5, 2, 2);
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(24, 4);
    _display.println("REARME BLOQUEADO");
    _display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    _display.setCursor(0, 26);
    _display.println(motivo);
    _display.setCursor(0, 38);
    _display.print("Bloqueios: ");
    _display.print(totalBloqueios);
    _display.drawLine(0, 46, 128, 46, SSD1306_WHITE);
    _display.setCursor(0, 52);
    _display.print("UID: ");
    _display.print(uid);
    _display.display();
}

void DisplayOLED::_desenharIcone(int tipo, int x, int y)
{
    switch (tipo)
    {
    case 1:
        _display.drawRect(x, y + 6, 12, 9, SSD1306_WHITE);
        _display.fillRect(x + 2, y + 8, 8, 5, SSD1306_WHITE);
        _display.drawCircle(x + 6, y + 6, 4, SSD1306_WHITE);
        break;
    case 2:
        _display.drawTriangle(x + 8, y, x, y + 14, x + 16, y + 14, SSD1306_WHITE);
        _display.drawLine(x + 8, y + 5, x + 8, y + 9, SSD1306_WHITE);
        _display.drawPixel(x + 8, y + 12, SSD1306_WHITE);
        break;
    case 3:
        _display.drawRect(x, y + 6, 12, 9, SSD1306_WHITE);
        _display.drawRect(x + 2, y + 8, 8, 5, SSD1306_WHITE);
        _display.drawCircle(x + 6, y + 3, 4, SSD1306_WHITE);
        _display.fillRect(x + 2, y + 2, 5, 4, SSD1306_BLACK);
        _display.drawLine(x + 19, y + 2, x + 19, y + 8, SSD1306_WHITE);
        _display.drawPixel(x + 19, y + 11, SSD1306_WHITE);
        break;
    case 4:
        _display.drawTriangle(x + 8, y, x, y + 14, x + 16, y + 14, SSD1306_WHITE);
        _display.drawLine(x + 4, y + 7, x + 8, y + 11, SSD1306_WHITE);
        _display.drawLine(x + 8, y + 11, x + 12, y + 5, SSD1306_WHITE);
        break;
    case 5:
        _display.drawRect(x, y + 2, 12, 16, SSD1306_WHITE);
        _display.fillRect(x + 8, y + 9, 2, 3, SSD1306_WHITE);
        _display.drawLine(x + 15, y + 5, x + 15, y + 12, SSD1306_WHITE);
        _display.drawPixel(x + 15, y + 15, SSD1306_WHITE);
        break;
    }
}