#include "test_mode.h"

void executarModoTeste()
{
    logSerial("INFO", "TESTE", "=== INICIANDO AUTODIAGNOSTICO v3 ===");
    enviarLogSite("TESTE: Iniciando autodiagnostico...");

    int ok = 0, falhas = 0;
    String resumoFalhas = "";

    if (oled.getDisplay().begin(SSD1306_SWITCHCAPVCC, ENDERECO_OLED))
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

    oled.getDisplay().clearDisplay();
    oled.getDisplay().setTextSize(1);
    oled.getDisplay().setTextColor(SSD1306_WHITE);
    oled.getDisplay().setCursor(20, 20);
    oled.getDisplay().println("MODO TESTE");
    oled.getDisplay().setCursor(10, 40);
    oled.getDisplay().println("Aguarde...");
    oled.getDisplay().display();

    byte versao = leitorRFID.getRFID().PCD_ReadRegister(MFRC522::VersionReg);
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

    oled.getDisplay().clearDisplay();
    oled.getDisplay().setTextSize(1);
    oled.getDisplay().setTextColor(SSD1306_WHITE);
    oled.getDisplay().setCursor(0, 0);
    oled.getDisplay().println("TESTE CONCLUIDO");
    oled.getDisplay().drawLine(0, 12, 128, 12, SSD1306_WHITE);
    oled.getDisplay().setCursor(0, 20);
    oled.getDisplay().print("OK:     ");
    oled.getDisplay().println(ok);
    oled.getDisplay().setCursor(0, 32);
    oled.getDisplay().print("FALHAS: ");
    oled.getDisplay().println(falhas);
    oled.getDisplay().display();

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