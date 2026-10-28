#include "serial_commands.h"
#include <Arduino.h>

void processarComandoSerial(String cmd)
{
    cmd.trim();
    cmd.toUpperCase();
    logSerial("INFO", "CMD", "Comando recebido: " + cmd);

    if (cmd.startsWith("SET LIMIAR "))
    {
        float novo = cmd.substring(11).toFloat();
        if (novo >= 5 && novo <= 400)
        {
            alarme.limiarDistancia = novo;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar ajustado para " + String(novo) + " cm.");
        }
        else
        {
            logSerial("WARN", "CMD", "Valor fora da faixa (5-400 cm).");
        }
    }
    else if (cmd.startsWith("SET TIMEOUT "))
    {
        long minutos = cmd.substring(12).toInt();
        if (minutos >= 0 && minutos <= 1440)
        {
            alarme.timeoutDesarme = (unsigned long)minutos * 60UL * 1000UL;
            salvarConfig();
            logSerial("INFO", "CMD", "Timeout de desarme: " + String(minutos) + " min.");
            if (alarme.modo == DESARMADO)
            {
                alarme.tempoInicioDesarme = millis();
                alarme.aviso5minDisparado = false;
            }
        }
        else
        {
            logSerial("WARN", "CMD", "Timeout fora da faixa (0-1440 min).");
        }
    }
    else if (cmd.startsWith("SET TEMP_ALERTA "))
    {
        float v = cmd.substring(16).toFloat();
        if (v >= 15 && v <= 50)
        {
            limiarTempAlerta = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar temp alerta: " + String(v, 1) + "C");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (15-50).");
    }
    else if (cmd.startsWith("SET TEMP_CRIT "))
    {
        float v = cmd.substring(14).toFloat();
        if (v >= 20 && v <= 60)
        {
            limiarTempCritico = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar temp critico: " + String(v, 1) + "C");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (20-60).");
    }
    else if (cmd.startsWith("SET UMID_ALERTA "))
    {
        float v = cmd.substring(16).toFloat();
        if (v >= 30 && v <= 90)
        {
            limiarUmidAlerta = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar umid alerta: " + String(v, 1) + "%");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (30-90).");
    }
    else if (cmd.startsWith("SET UMID_CRIT "))
    {
        float v = cmd.substring(14).toFloat();
        if (v >= 40 && v <= 100)
        {
            limiarUmidCritico = v;
            salvarConfig();
            logSerial("INFO", "CMD", "Limiar umid critico: " + String(v, 1) + "%");
        }
        else
            logSerial("WARN", "CMD", "Fora da faixa (40-100).");
    }
    else if (cmd == "STATUS")
    {
        Serial.println(F("----------- STATUS ATUAL -----------"));
        Serial.print(F("  Modo Vigilante:       "));
        Serial.println(alarme.nomeModo());
        Serial.print(F("  Estado Alarme:        "));
        Serial.println(alarme.nomeEstadoAtual());
        Serial.print(F("  Porta (filtrada):     "));
        Serial.println(porta.estaAberta() ? "ABERTA" : "fechada");
        Serial.print(F("  Timeout Desarme:      "));
        Serial.print(alarme.timeoutDesarme / 60000UL);
        Serial.println(F(" min"));
        if (alarme.modo == DESARMADO)
        {
            unsigned long dec = millis() - alarme.tempoInicioDesarme;
            unsigned long rst = (alarme.timeoutDesarme > dec) ? (alarme.timeoutDesarme - dec) / 1000UL : 0;
            Serial.print(F("  Rearme em:            "));
            Serial.print(rst);
            Serial.println(F(" s"));
            Serial.print(F("  UID Desarme:          "));
            Serial.println(alarme.uidUltimoDesarme);
        }
        else
        {
            Serial.print(F("  UID Ultimo Rearme:    "));
            Serial.println(alarme.uidUltimoRearme.length() ? alarme.uidUltimoRearme : "-");
        }
        Serial.print(F("  Contador Desarmes:    "));
        Serial.println(alarme.contadorDesarmes);
        Serial.print(F("  Rearmes Bloqueados:   "));
        Serial.println(alarme.contadorRearmesBloqueados);
        Serial.print(F("  Duracao ult. desarme: "));
        Serial.print(alarme.duracaoUltimoDesarme);
        Serial.println(F(" s"));
        Serial.print(F("  Limiar Distancia:     "));
        Serial.print(alarme.limiarDistancia);
        Serial.println(F(" cm"));
        Serial.print(F("  Aberturas da Porta:   "));
        Serial.println(alarme.contadorAberturasPorta);
        Serial.print(F("  Acessos RFID:         "));
        Serial.println(alarme.contadorAcessosValidos);
        Serial.print(F("  Invasoes:             "));
        Serial.println(alarme.contadorInvasoes);
        Serial.print(F("  Aproximacoes:         "));
        Serial.println(alarme.contadorAproximacoes);
        Serial.print(F("  Mov. em manutencao:   "));
        Serial.println(alarme.contadorMovimentosManutencao);
        Serial.print(F("  Limiares ambientais:  T["));
        Serial.print(limiarTempAlerta, 1);
        Serial.print(F("/"));
        Serial.print(limiarTempCritico, 1);
        Serial.print(F("C] U["));
        Serial.print(limiarUmidAlerta, 1);
        Serial.print(F("/"));
        Serial.print(limiarUmidCritico, 1);
        Serial.println(F("%]"));
        Serial.print(F("  Temp/umid atuais:     T="));
        Serial.print(_ultimaTempValida, 1);
        Serial.print(F("C U="));
        Serial.print(_ultimaUmidValida, 1);
        Serial.println(F("%"));
        Serial.print(F("  Fonte de tempo:       "));
        Serial.print(relogio.fonteTempoStr());
        Serial.print(F(" | Hora: "));
        Serial.println(relogio.timestamp());
        Serial.print(F("  Historico (buffer):   "));
        Serial.print(logBuffer.tamanho());
        Serial.print(F("/"));
        Serial.println(HIST_TAMANHO);
        Serial.print(F("  WiFi:                 "));
        Serial.println(WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado");
        Serial.print(F("  MQTT:                 "));
        Serial.println(notificador.estaConectado() ? "conectado" : "desconectado");
        Serial.print(F("  OTA:                  "));
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.print(OTA_HOSTNAME);
            Serial.print(":");
            Serial.print(OTA_PORT);
            Serial.println(F(" (ativo)"));
        }
        else
        {
            Serial.println(F("(Wi-Fi offline)"));
        }
        Serial.println(F("-------------------------------------"));
    }
    else if (cmd == "OTA")
    {
        Serial.println(F("----------- STATUS OTA -----------"));
        Serial.print(F("  Hostname:  "));
        Serial.println(OTA_HOSTNAME);
        Serial.print(F("  Porta:     "));
        Serial.println(OTA_PORT);
        Serial.print(F("  Senha:     "));
        Serial.println(OTA_PASSWORD);
        Serial.print(F("  IP:        "));
        if (WiFi.status() == WL_CONNECTED)
            Serial.println(WiFi.localIP());
        else
            Serial.println(F("Wi-Fi offline"));
        Serial.print(F("  Ativo:     "));
        Serial.println(WiFi.status() == WL_CONNECTED ? "sim" : "nao (Wi-Fi offline)");
        Serial.println(F("-----------------------------------"));
        Serial.println(F("  No Arduino IDE: Ferramentas > Porta > rede"));
        Serial.println(F("  Selecione o dispositivo e faca upload normalmente."));
    }
    else if (cmd == "HIST")
    {
        logBuffer.dumpSerial();
    }
    else if (cmd == "HIST_CLEAR")
    {
        logBuffer.clear();
        logSerial("INFO", "CMD", "Comando HIST_CLEAR executado.");
    }
    else if (cmd == "HIST_JSON")
    {
        logBuffer.dumpJSON();
    }
    else if (cmd == "ARMAR")
    {
        if (alarme.modo == DESARMADO)
        {
            logSerial("INFO", "CMD", "Tentativa de ARMAR via Serial.");
            alarme.armarViaSerial(porta, audio);
            salvarConfig();
        }
        else
        {
            logSerial("WARN", "CMD", "Ja esta armado.");
        }
    }
    else if (cmd == "DESARMAR")
    {
        if (alarme.modo == ARMADO)
        {
            alarme.modo = DESARMADO;
            alarme.tempoInicioDesarme = millis();
            alarme.uidUltimoDesarme = "SERIAL";
            alarme.aviso5minDisparado = false;
            alarme.contadorDesarmes++;
            contadoresDirty = true;
            prefs.begin("rack", false);
            prefs.putUInt("modo", (uint8_t)DESARMADO);
            prefs.end();
            logSerial("INFO", "CMD", "Modo Vigilante DESARMADO via Serial.");
            salvarConfig();
        }
        else
        {
            logSerial("WARN", "CMD", "Ja esta desarmado.");
        }
    }
    else if (cmd == "TESTE")
    {
        logSerial("INFO", "CMD", "Executando modo teste via Serial.");
        alarme.modoTestePendente = true;
    }
    else if (cmd == "PANICO")
    {
        logSerial("INFO", "CMD", "Executando panico via Serial.");
        alarme.dispararPanico("SERIAL");
    }
    else if (cmd == "RESET")
    {
        alarme.contadorAberturasPorta = 0;
        alarme.contadorAproximacoes = 0;
        alarme.contadorAcessosValidos = 0;
        alarme.contadorMovimentosManutencao = 0;
        alarme.contadorDesarmes = 0;
        salvarConfig();
        logSerial("INFO", "CMD", "Contadores operacionais zerados (seguranca preservada).");
    }
    else if (cmd.startsWith("RTC "))
    {
        int ano, mes, dia, hora, min, seg;
        if (sscanf(cmd.c_str(), "RTC %d-%d-%d %d:%d:%d",
                   &ano, &mes, &dia, &hora, &min, &seg) == 6)
        {
            if (relogio.ajustarManual(ano, mes, dia, hora, min, seg))
            {
                logSerial("INFO", "RTC", "Ajuste manual OK. Fonte: " + relogio.fonteTempoStr());
            }
        }
        else
        {
            logSerial("WARN", "CMD", "Formato: RTC YYYY-MM-DD HH:MM:SS");
        }
    }
    else if (cmd == "RTC")
    {
        logSerial("INFO", "RTC", "Hora atual: " + relogio.timestamp() + " | Fonte: " + relogio.fonteTempoStr());
    }
    else if (cmd == "NTP")
    {
        if (relogio.sincronizarNTP())
        {
            logSerial("INFO", "NTP", "Sync manual OK. Fonte: " + relogio.fonteTempoStr());
        }
        else
        {
            logSerial("WARN", "NTP", "Sync manual falhou.");
        }
    }
    else if (cmd.startsWith("ADD_CARD "))
    {
        String novoUID = cmd.substring(9);
        novoUID.trim();
        if (novoUID.length() >= 8)
        {
            bool jaExiste = false;
            for (const String &c : cartoesAutorizados)
            {
                if (c == novoUID)
                {
                    jaExiste = true;
                    break;
                }
            }
            if (jaExiste)
            {
                logSerial("WARN", "CMD", "Cartao ja cadastrado: " + novoUID);
            }
            else
            {
                prefs.begin("rack", false);
                int total = prefs.getInt("total_cards", 0);
                String chave = "card_" + String(total);
                prefs.putString(chave.c_str(), novoUID);
                prefs.putInt("total_cards", total + 1);
                prefs.end();
                cartoesAutorizados.push_back(novoUID);
                logSerial("INFO", "CMD", "Cartao cadastrado na NVS: " + novoUID);
            }
        }
        else
        {
            logSerial("WARN", "CMD", "UID invalido. Use ADD_CARD <UID_HEX> (min 8 chars)");
        }
    }
    else if (cmd == "LIST_CARDS")
    {
        Serial.println(F("--- Cartoes Autorizados ---"));
        for (const String &card : cartoesAutorizados)
            Serial.println(card);
        Serial.println(F("---------------------------"));
    }
    else if (cmd == "HELP")
    {
        Serial.println(F("----------- COMANDOS DISPONIVEIS -----------"));
        Serial.println(F("  SET LIMIAR <5-400>      - Ajusta limiar ultrassonico (persiste)"));
        Serial.println(F("  SET TIMEOUT <0-1440>    - Timeout de desarme (min, persiste)"));
        Serial.println(F("  SET TEMP_ALERTA <15-50> - Limiar temp alerta (C, persiste)"));
        Serial.println(F("  SET TEMP_CRIT <20-60>   - Limiar temp critico (C, persiste)"));
        Serial.println(F("  SET UMID_ALERTA <30-90> - Limiar umid alerta (%, persiste)"));
        Serial.println(F("  SET UMID_CRIT <40-100>  - Limiar umid critico (%, persiste)"));
        Serial.println(F("  STATUS                  - Status completo"));
        Serial.println(F("  ARMAR                   - Forca ARMADO (se porta fechada)"));
        Serial.println(F("  DESARMAR                - Forca DESARMADO"));
        Serial.println(F("  TESTE                   - Modo teste"));
        Serial.println(F("  PANICO                  - Dispara panico manual"));
        Serial.println(F("  RTC                     - Mostra hora e fonte de tempo"));
        Serial.println(F("  RTC YYYY-MM-DD HH:MM:SS - Ajusta RTC manualmente"));
        Serial.println(F("  NTP                     - Forca sincronizacao NTP manual"));
        Serial.println(F("  OTA                     - Mostra status do servico OTA"));
        Serial.println(F("  HIST                    - Despeja historico de eventos (Serial)"));
        Serial.println(F("  HIST_JSON               - Despeja historico em JSON"));
        Serial.println(F("  HIST_CLEAR              - Limpa o historico de eventos"));
        Serial.println(F("  RESET                   - Zera contadores operacionais (preserva seguranca)"));
        Serial.println(F("  ADD_CARD <UID>          - Cadastra novo cartao na NVS"));
        Serial.println(F("  LIST_CARDS              - Lista cartoes autorizados"));
        Serial.println(F("  HELP                    - Lista comandos"));
        Serial.println(F("---------------------------------------------"));
        Serial.println(F("  BOTAO : curto=reset | longo=rearme | duplo=teste"));
        Serial.println(F("  PANICO: GPIO 26 (pushbutton para GND) - ISR, resposta <30ms"));
        Serial.println(F("  OTA   : Arduino IDE > Ferramentas > Porta > rede"));
        Serial.println(F("  HIST  : buffer circular de 100 eventos em RAM (perdido no reboot)"));
        Serial.println(F("  MQTT  : rack/status (pub) | rack/comando (sub seguranca)"));
        Serial.println(F("  MQTT  : rack/ambiente (pub retain 30s) | rack/comando/ambiente (sub)"));
        Serial.println(F("  MQTT  : rack/status/lwt | rack/logs | rack/heartbeat"));
        Serial.println(F("  MQTT_CMD seg: ARMAR | DESARMAR | STATUS | RESET <token> | TESTE"));
        Serial.println(F("  MQTT_CMD amb: SET_TEMP_ALERTA | SET_TEMP_CRIT | SET_UMID_ALERTA | SET_UMID_CRIT | PANICO | STATUS_AMB"));
        Serial.println(F("  NTP: pool.ntp.org | time.google.com | time.cloudflare.com"));
    }
    else
    {
        logSerial("WARN", "CMD", "Comando desconhecido. Digite HELP.");
    }
}