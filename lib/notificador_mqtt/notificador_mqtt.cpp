#include "notificador_mqtt.h"

// Se o prefs ainda estiver no main.cpp, declaramos aqui:
extern Preferences prefs;

void NotificadorMQTT::begin()
{
    Serial.print(F("[MQTT] Conectando ao WiFi"));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 6);
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < 15000)
    {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println(F(" OK!"));
        Serial.print(F("[MQTT] IP: "));
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println(F(" FALHOU (WiFi indisponivel, seguindo offline)"));
    }

    _client.setServer(MQTT_BROKER, MQTT_PORT);
    _client.setBufferSize(512);
    _client.setKeepAlive(120);

    _client.setCallback([this](char *topic, byte *payload, unsigned int length)
                        { _tratarComando(topic, payload, length); });
}

void NotificadorMQTT::update()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - _ultimoTempoReconexao > MQTT_RETRY_MS)
        {
            _ultimoTempoReconexao = millis();
            WiFi.reconnect();
        }
        return;
    }

    if (!_client.connected())
    {
        if (_estavaConectado)
        {
            logSerial("WARN", "MQTT", "Desconectado do broker (rc=" + String(_client.state()) + ").");
            _estavaConectado = false;
        }

        if (millis() - _ultimoTempoReconexao > MQTT_RETRY_MS)
        {
            _ultimoTempoReconexao = millis();
            _client.disconnect();
            String clientId = "RackESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            Serial.print(F("[MQTT] Conectando ao broker... "));

            if (_client.connect(clientId.c_str(), nullptr, nullptr,
                                MQTT_TOPIC_LWT, 1, true, "{\"status\":\"offline\"}"))
            {
                Serial.println(F("OK"));
                _estavaConectado = true;
                _client.publish(MQTT_TOPIC_LWT, "{\"status\":\"online\"}", true);
                _client.subscribe(MQTT_TOPIC_CMD);
                _client.subscribe(MQTT_TOPIC_CMD_AMBIENTE);
                logSerial("INFO", "MQTT", String("Inscrito em: ") + MQTT_TOPIC_CMD + " e " + MQTT_TOPIC_CMD_AMBIENTE);
                _precisaPublicarInicial = true;
            }
            else
            {
                Serial.print(F("Falha rc="));
                Serial.println(_client.state());
            }
        }
        return;
    }

    _client.loop();

    if (_precisaPublicarInicial)
    {
        _precisaPublicarInicial = false;
        _publicar(true);
        _publicarAmbiente();
    }

    if (alarme.estadoAtual != _ultimoEstadoPublicado ||
        alarme.modo != _ultimoModoPublicado ||
        porta.estaAberta() != _ultimoEstadoPortaPublicado)
    {
        _publicar();
    }

    if (millis() - _ultimoHeartbeat > INTERVALO_HEARTBEAT_MS)
    {
        _ultimoHeartbeat = millis();
        String hbPayload = "{\"ts\":\"" + relogio.timestampISO() +
                           "\",\"uptime_s\":" + String(millis() / 1000) +
                           ",\"time_source\":\"" + relogio.fonteTempoStr() + "\"}";
        _client.publish(MQTT_TOPIC_HB, hbPayload.c_str());
    }

    if (millis() - _ultimoAmbientePublicado > INTERVALO_AMBIENTE_MS)
    {
        _ultimoAmbientePublicado = millis();
        _publicarAmbiente();
    }
}

bool NotificadorMQTT::estaConectado() { return _client.connected(); }

bool NotificadorMQTT::publicarLog(const String &payload)
{
    return _client.publish(MQTT_TOPIC_LOGS, payload.c_str());
}

void NotificadorMQTT::_publicar(bool forcar)
{
    if (!forcar && alarme.estadoAtual == _ultimoEstadoPublicado &&
        alarme.modo == _ultimoModoPublicado &&
        porta.estaAberta() == _ultimoEstadoPortaPublicado)
    {
        return;
    }

    bool ativo = (alarme.modo == ARMADO &&
                  (alarme.estadoAtual == EMERGENCIA_RACK ||
                   alarme.estadoAtual == INVASAO_SALA));

    String payload = "{\"estado\":\"";
    payload += alarme.nomeEstadoAtual();
    payload += "\",\"modo\":\"";
    payload += alarme.nomeModo();
    payload += "\",\"ts\":\"";
    payload += relogio.timestampISO();
    payload += "\",\"time_source\":\"";
    payload += relogio.fonteTempoStr();
    payload += "\"";
    payload += ",\"porta\":";
    payload += (porta.estaAberta() ? "true" : "false");
    payload += ",\"luz\":";
    payload += (ldr.estaAcesa() ? "true" : "false");
    float dist = ultrassonico.distancia();
    payload += ",\"dist_cm\":";
    if (dist < 0.0f)
        payload += "null";
    else
        payload += String(dist, 1);
    payload += ",\"ativo\":";
    payload += (ativo ? "true" : "false");

    payload += ",\"ip\":\"";
    payload += WiFi.localIP().toString();
    payload += "\",\"uid\":\"";
    payload += (alarme.uidUltimoDesarme.length() > 0 ? alarme.uidUltimoDesarme : "-");
    payload += "\",\"acessos\":";
    payload += String(alarme.contadorAcessosValidos);
    payload += ",\"invasoes\":";
    payload += String(alarme.contadorInvasoes);
    payload += ",\"rearmes\":";
    payload += String(alarme.contadorRearmesBloqueados);

    payload += ",\"alerta_temp\":";
    payload += (alertaTempAtivo ? "true" : "false");
    payload += ",\"critico_temp\":";
    payload += (criticoTempAtivo ? "true" : "false");
    payload += ",\"alerta_umid\":";
    payload += (alertaUmidAtivo ? "true" : "false");
    payload += ",\"critico_umid\":";
    payload += (criticoUmidAtivo ? "true" : "false");
    payload += ",\"taxa_temp_alerta\":";
    payload += (taxaTempAlerta ? "true" : "false");
    payload += ",\"ambiente\":\"";
    payload += (estadoAmbiental == AMB_CRITICO ? "CRITICO" : estadoAmbiental == AMB_ALERTA ? "ALERTA"
                                                                                           : "OK");
    payload += "\"";

    payload += ",\"temp_c\":" + String(_ultimaTempValida, 1);
    payload += ",\"umid_pct\":" + String(_ultimaUmidValida, 1);
    payload += "}";

    Serial.print(F("[MQTT] Publicando em "));
    Serial.print(MQTT_TOPIC_STATUS);
    Serial.print(F(": "));
    Serial.println(payload);

    bool ok = _client.publish(MQTT_TOPIC_STATUS, payload.c_str(), true);
    if (!ok)
    {
        Serial.println(F("[ERRO] Falha ao publicar status! Buffer cheio?"));
        return;
    }

    _ultimoEstadoPublicado = alarme.estadoAtual;
    _ultimoModoPublicado = alarme.modo;
    _ultimoEstadoPortaPublicado = porta.estaAberta();
}

void NotificadorMQTT::_publicarAmbiente()
{
    String payload = "{";
    payload += "\"ts\":\"" + relogio.timestampISO() + "\"";
    payload += ",\"time_source\":\"" + relogio.fonteTempoStr() + "\"";
    payload += ",\"temp_c\":" + String(_ultimaTempValida, 1);
    payload += ",\"umid_pct\":" + String(_ultimaUmidValida, 1);
    payload += ",\"estado\":\"";
    payload += (estadoAmbiental == AMB_CRITICO ? "CRITICO" : estadoAmbiental == AMB_ALERTA ? "ALERTA"
                                                                                           : "OK");
    payload += "\",\"alerta_temp\":";
    payload += (alertaTempAtivo ? "true" : "false");
    payload += ",\"critico_temp\":";
    payload += (criticoTempAtivo ? "true" : "false");
    payload += ",\"alerta_umid\":";
    payload += (alertaUmidAtivo ? "true" : "false");
    payload += ",\"critico_umid\":";
    payload += (criticoUmidAtivo ? "true" : "false");
    payload += ",\"taxa_temp_alerta\":";
    payload += (taxaTempAlerta ? "true" : "false");
    payload += ",\"sensor_dht_ok\":";
    payload += (_sensorDHTPerdido ? "false" : "true");
    payload += ",\"limiares\":{\"t_alert\":" + String(limiarTempAlerta, 1) +
               ",\"t_crit\":" + String(limiarTempCritico, 1) +
               ",\"u_alert\":" + String(limiarUmidAlerta, 1) +
               ",\"u_crit\":" + String(limiarUmidCritico, 1) + "}";
    payload += "}";

    bool ok = _client.publish(MQTT_TOPIC_AMBIENTE, payload.c_str(), true);
    if (!ok)
    {
        Serial.println(F("[WARN] Falha ao publicar ambiente (buffer cheio?)"));
    }
}

void NotificadorMQTT::_tratarComando(char *topic, byte *payload, unsigned int length)
{
    String cmd;
    for (unsigned int i = 0; i < length; i++)
        cmd += (char)payload[i];
    cmd.trim();

    String cmdOriginal = cmd;
    cmd.toUpperCase();

    logSerial("INFO", "MQTT_CMD", String("Recebido [") + topic + "]: " + cmd);

    if (String(topic) == MQTT_TOPIC_CMD_AMBIENTE)
    {
        if (cmd.startsWith("SET_TEMP_ALERTA "))
        {
            float v = cmd.substring(16).toFloat();
            if (v >= 15 && v <= 50)
            {
                limiarTempAlerta = v;
                salvarConfig();
                logSerial("INFO", "MQTT_AMB", "Temp alerta: " + String(v, 1) + "C");
                _publicarAmbiente();
            }
            else
                logSerial("WARN", "MQTT_AMB", "Fora da faixa (15-50).");
        }
        else if (cmd.startsWith("SET_TEMP_CRIT "))
        {
            float v = cmd.substring(14).toFloat();
            if (v >= 20 && v <= 60)
            {
                limiarTempCritico = v;
                salvarConfig();
                logSerial("INFO", "MQTT_AMB", "Temp critico: " + String(v, 1) + "C");
                _publicarAmbiente();
            }
            else
                logSerial("WARN", "MQTT_AMB", "Fora da faixa (20-60).");
        }
        else if (cmd.startsWith("SET_UMID_ALERTA "))
        {
            float v = cmd.substring(16).toFloat();
            if (v >= 30 && v <= 90)
            {
                limiarUmidAlerta = v;
                salvarConfig();
                logSerial("INFO", "MQTT_AMB", "Umid alerta: " + String(v, 1) + "%");
                _publicarAmbiente();
            }
            else
                logSerial("WARN", "MQTT_AMB", "Fora da faixa (30-90).");
        }
        else if (cmd.startsWith("SET_UMID_CRIT "))
        {
            float v = cmd.substring(14).toFloat();
            if (v >= 40 && v <= 100)
            {
                limiarUmidCritico = v;
                salvarConfig();
                logSerial("INFO", "MQTT_AMB", "Umid critico: " + String(v, 1) + "%");
                _publicarAmbiente();
            }
            else
                logSerial("WARN", "MQTT_AMB", "Fora da faixa (40-100).");
        }
        else if (cmd == "PANICO")
        {
            alarme.dispararPanico("MQTT");
        }
        else if (cmd == "STATUS_AMB")
        {
            _publicarAmbiente();
        }
        else
        {
            logSerial("WARN", "MQTT_AMB", "Comando ambiente desconhecido: " + cmd);
        }
        return;
    }

    if (cmd == "DESARMAR")
    {
        if (alarme.modo == ARMADO)
        {
            alarme.modo = DESARMADO;
            alarme.tempoInicioDesarme = millis();
            alarme.uidUltimoDesarme = "MQTT";
            alarme.aviso5minDisparado = false;
            alarme.contadorDesarmes++;
            contadoresDirty = true;
            prefs.begin("rack", false);
            prefs.putUInt("modo", (uint8_t)DESARMADO);
            prefs.end();
            logSerial("INFO", "MQTT_CMD", "Desarmado remoto executado.");
            enviarLogSite("Comando remoto: desarmar executado");
        }
        else
        {
            logSerial("WARN", "MQTT_CMD", "Ja desarmado, ignorando.");
            enviarLogSite("Comando remoto: desarmar (ja desarmado)");
        }
    }
    else if (cmd == "ARMAR")
    {
        if (alarme.modo == DESARMADO)
        {
            if (alarme.armarViaSerial(porta, audio))
            {
                logSerial("INFO", "MQTT_CMD", "Armado remoto executado.");
                enviarLogSite("Comando remoto: armar executado");
            }
            else
            {
                logSerial("WARN", "MQTT_CMD", "Armar remoto bloqueado (porta aberta).");
                enviarLogSite("Comando remoto: armar bloqueado");
            }
        }
        else
        {
            logSerial("WARN", "MQTT_CMD", "Ja armado, ignorando.");
            enviarLogSite("Comando remoto: armar (ja armado)");
        }
    }
    else if (cmd == "STATUS")
    {
        if (millis() - _ultimoStatusForcado < STATUS_RATE_LIMIT_MS)
        {
            logSerial("WARN", "MQTT_CMD", "STATUS ignorado (rate limit).");
            return;
        }
        _ultimoStatusForcado = millis();
        logSerial("INFO", "MQTT_CMD", "Enviando STATUS sob demanda.");
        _publicar(true);
    }
    else if (cmd.startsWith("RESET"))
    {
        String token = cmdOriginal.substring(5);
        token.trim();

        if (token != MQTT_RESET_TOKEN)
        {
            logSerial("WARN", "MQTT_CMD", "RESET negado: token invalido.");
            enviarLogSite("Comando remoto: reset negado (token invalido)");
        }
        else
        {
            alarme.contadorAberturasPorta = 0;
            alarme.contadorAproximacoes = 0;
            alarme.contadorAcessosValidos = 0;
            alarme.contadorMovimentosManutencao = 0;
            alarme.contadorDesarmes = 0;
            alarme.resetContadoresPendente = false;
            salvarConfig();
            logSerial("INFO", "MQTT_CMD", "Reset remoto executado (seguranca preservada).");
            enviarLogSite("Comando remoto: reset operacional executado");
            _publicar(true);
        }
    }
    else if (cmd == "TESTE")
    {
        alarme.modoTestePendente = true;
        logSerial("INFO", "MQTT_CMD", "Modo teste remoto executado.");
        enviarLogSite("Comando remoto: teste iniciado");
    }
    else
    {
        logSerial("WARN", "MQTT_CMD", "Comando desconhecido: " + cmd);
    }
}