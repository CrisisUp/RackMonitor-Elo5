#include "alarme.h"
#include "porta.h" // Necessário para acessar Porta::estaAberta()

void Alarme::update(const LeiturasSensores &s, EventoRFID rfidEv, EventoBotao botaoEv,
                    Porta &porta, Audio &audio)
{
    _logBordas(s);
    _logPorta(porta);

    if (rfidEv.novo)
    {
        ultimoUIDtempoRFID = millis();
        _tratarRFID(rfidEv, porta, audio);
    }

    if (botaoEv != BOTAO_NENHUM)
    {
        _tratarBotao(botaoEv, porta, audio);
    }

    if (modo == DESARMADO)
    {
        unsigned long decorrido = millis() - tempoInicioDesarme;
        unsigned long restante = (timeoutDesarme > decorrido) ? (timeoutDesarme - decorrido) : 0;

        if (!aviso5minDisparado && restante <= 5UL * 60UL * 1000UL &&
            timeoutDesarme > 5UL * 60UL * 1000UL)
        {
            aviso5minDisparado = true;
            logSerial("WARN", "VIGILANTE", "AVISO: 5 minutos para rearme automatico!");
            audio.beepCurto(1500, 200);
        }

        if (decorrido >= timeoutDesarme)
            _rearmarPorTimeout(porta, audio);
    }

    if (motivoBloqueio.length() > 0 && (millis() - tempoBloqueioAtivo) > DURACAO_AVISO_BLOQUEIO_MS)
    {
        motivoBloqueio = "";
    }

    if (modo == ARMADO)
        _atualizarEstados(s, rfidEv);
}

bool Alarme::bloqueioAtivo() const { return motivoBloqueio.length() > 0; }

void Alarme::dispararPanico(const String &origem)
{
    if (modo == DESARMADO)
    {
        logSerial("WARN", "PANICO", "Botao de panico ignorado: sistema DESARMADO (origem: " + origem + ").");
        enviarLogSite("Panico ignorado: sistema desarmado.");
        audio.beepErro();
        return;
    }
    if (estadoAtual == INVASAO_SALA)
    {
        logSerial("WARN", "PANICO", "Ja em INVASAO_SALA, ignorando novo disparo.");
        return;
    }

    estadoAnterior = estadoAtual;
    estadoAtual = INVASAO_SALA;
    estadoCandidato = INVASAO_SALA;
    tempoEstadoCandidato = millis();

    contadorInvasoes++;
    contadoresDirty = true;

    logSerial("CRIT", "PANICO", "BOTAO DE PANICO ACIONADO (origem: " + origem + ") | Invasao #" + String(contadorInvasoes));
    enviarLogSite("CRITICO: Botao de panico acionado! Invasao #" + String(contadorInvasoes));
}

bool Alarme::armarViaSerial(Porta &porta, Audio &audio)
{
    if (modo == ARMADO)
        return false;
    return _tentarArmar(porta, audio);
}

String Alarme::nomeModo() const { return (modo == ARMADO) ? "ARMADO" : "DESARMADO"; }
String Alarme::nomeEstadoAtual() const { return _nomeEstado(estadoAtual); }

// ==========================================
// MÉTODOS PRIVADOS
// ==========================================

void Alarme::_logBordas(const LeiturasSensores &s)
{
    if (_primeiraLeitura)
    {
        _estadoAnteriorPIR = s.movimento;
        _estadoAnteriorLDR = s.luzAcesa;
        _primeiraLeitura = false;
        return;
    }

    if (s.movimento != _estadoAnteriorPIR)
    {
        if (modo == DESARMADO && s.movimento)
        {
            contadorMovimentosManutencao++;
            contadoresDirty = true;
            logSerial("INFO", "MANUT", "Movimento em modo manutencao (auditoria).");
        }
        else
        {
            logSerial("INFO", "SENSOR", s.movimento ? "Movimento detectado pelo PIR." : "Movimento cessou.");
        }
        _estadoAnteriorPIR = s.movimento;
    }
    if (s.luzAcesa != _estadoAnteriorLDR)
    {
        logSerial("INFO", "SENSOR", s.luzAcesa ? "Luzes da sala ACESAS." : "Luzes da sala APAGADAS.");
        _estadoAnteriorLDR = s.luzAcesa;
    }
}

void Alarme::_logPorta(Porta &porta)
{
    bool aberta = porta.estaAberta();
    if (aberta != _portaEstadoAnterior)
    {
        if (aberta)
        {
            contadorAberturasPorta++;
            contadoresDirty = true;
            if (modo == ARMADO)
            {
                logSerial("ALERT", "PORTA", "Porta do rack aberta (total: " + String(contadorAberturasPorta) + ")");
                enviarLogSite("Porta do rack aberta");
            }
            else
            {
                logSerial("INFO", "PORTA", "Porta aberta em manutencao (total: " + String(contadorAberturasPorta) + ")");
                enviarLogSite("Porta aberta (manutencao)");
            }
        }
        else
        {
            logSerial("INFO", "PORTA", "Porta fechada");
            enviarLogSite("Porta do rack fechada");
        }
        _portaEstadoAnterior = aberta;
    }
}

void Alarme::_tratarRFID(const EventoRFID &ev, Porta &porta, Audio &audio)
{
    uidUltimoDesarme = ev.uid;

    bool valido = false;
    for (const String &card : cartoesAutorizados)
    {
        if (card == ev.uid)
        {
            valido = true;
            break;
        }
    }

    ultimoCartaoValido = valido;

    if (valido)
    {
        enviarLogSite("Cartao autorizado: " + ev.uid);
        contadorAcessosValidos++;
        contadoresDirty = true;
        logSerial("INFO", "RFID", "Cartao autorizado: " + ev.uid);
        _alternarModo(ev.uid, porta, audio);
    }
    else
    {
        logSerial("WARN", "RFID", "Cartao negado: " + ev.uid);
        enviarLogSite("Cartao negado: " + ev.uid);
        audio.beepErro();
    }
}

void Alarme::_tratarBotao(EventoBotao ev, Porta &porta, Audio &audio)
{
    switch (ev)
    {
    case BOTAO_LONGO:
        logSerial("INFO", "BOTAO", "Clique LONGO: rearme forcado.");
        if (modo == DESARMADO)
            _tentarArmar(porta, audio);
        else
            logSerial("WARN", "BOTAO", "Ja esta ARMADO.");
        audio.beepCurto(1000, 200);
        break;
    case BOTAO_DUPLO:
        logSerial("INFO", "BOTAO", "Clique DUPLO: modo teste.");
        modoTestePendente = true;
        break;
    case BOTAO_CURTO:
        logSerial("INFO", "BOTAO", "Clique CURTO: reset operacional.");
        resetContadoresPendente = true;
        audio.beepCurto(1200, 100);
        break;
    default:
        break;
    }
}

void Alarme::_alternarModo(const String &uid, Porta &porta, Audio &audio)
{
    if (modo == ARMADO)
    {
        modo = DESARMADO;
        tempoInicioDesarme = millis();
        uidUltimoDesarme = uid;
        aviso5minDisparado = false;
        contadorDesarmes++;
        contadoresDirty = true;
        motivoBloqueio = "";
        prefs.begin("rack", false);
        prefs.putUInt("modo", (uint8_t)DESARMADO);
        prefs.end();
        logSerial("INFO", "VIGILANTE", "DESARMADO por UID: " + uid + " | Timeout: " + String(timeoutDesarme / 60000) + " min");
        audio.beepSucesso();
    }
    else
    {
        _tentarArmar(porta, audio);
    }
}

bool Alarme::_tentarArmar(Porta &porta, Audio &audio)
{
    if (porta.estaAberta())
    {
        contadorRearmesBloqueados++;
        contadoresDirty = true;
        motivoBloqueio = "Feche a porta!";
        tempoBloqueioAtivo = millis();
        logSerial("WARN", "VIGILANTE", "Rearme bloqueado: porta aberta | " + String(contadorRearmesBloqueados) + " bloqueio(s) ate agora.");
        logSerial("WARN", "VIGILANTE", "Feche a porta e passe o cartao novamente para armar.");
        enviarLogSite("Rearme bloqueado: porta aberta");
        audio.beepErro();
        return false;
    }
    duracaoUltimoDesarme = (millis() - tempoInicioDesarme) / 1000UL;
    modo = ARMADO;
    uidUltimoRearme = uidUltimoDesarme;
    motivoBloqueio = "";
    prefs.begin("rack", false);
    prefs.putUInt("modo", (uint8_t)ARMADO);
    prefs.end();
    logSerial("INFO", "VIGILANTE", "Rearmado | Duracao do desarme: " + String(duracaoUltimoDesarme) + "s");
    enviarLogSite("Sistema rearmado com sucesso");
    audio.beepSucesso();
    estadoAtual = SEGURO;
    estadoAnterior = SEGURO;
    estadoCandidato = SEGURO;
    return true;
}

void Alarme::_rearmarPorTimeout(Porta &porta, Audio &audio)
{
    if (porta.estaAberta())
    {
        logSerial("WARN", "VIGILANTE", "TIMEOUT IGNORADO: porta aberta impede rearme automatico. Reiniciando contagem.");
        tempoInicioDesarme = millis();
        aviso5minDisparado = false;
        return;
    }
    duracaoUltimoDesarme = (millis() - tempoInicioDesarme) / 1000UL;
    modo = ARMADO;
    uidUltimoRearme = "TIMEOUT";
    prefs.begin("rack", false);
    prefs.putUInt("modo", (uint8_t)ARMADO);
    prefs.end();
    logSerial("WARN", "VIGILANTE", "Rearme automatico por timeout | Duracao: " + String(duracaoUltimoDesarme) + "s");
    audio.beepCurto(1500, 200);
    audio.beepCurto(2000, 300);
    estadoAtual = SEGURO;
    estadoAnterior = SEGURO;
    estadoCandidato = SEGURO;
}

void Alarme::_atualizarEstados(const LeiturasSensores &s, const EventoRFID &rfidEv)
{
    EstadoSistema candidato;
    bool rfidRecente = rfidEv.novo || (millis() - ultimoUIDtempoRFID < 2500);

    if (s.portaAberta)
    {
        candidato = EMERGENCIA_RACK;
    }
    else if (rfidRecente)
    {
        candidato = ACESSO_RFID;
    }
    else if (s.movimento)
    {
        candidato = AVISO_SALA;
    }
    else if (s.distancia < limiarDistancia && s.distancia > 0)
    {
        candidato = APROXIMACAO_RACK;
    }
    else
    {
        candidato = SEGURO;
    }

    bool critico = (candidato == EMERGENCIA_RACK);
    bool urgente = (candidato == ACESSO_RFID);

    if (critico || urgente)
    {
        estadoAtual = candidato;
        estadoCandidato = candidato;
        tempoEstadoCandidato = millis();
    }
    else
    {
        if (candidato != estadoCandidato)
        {
            estadoCandidato = candidato;
            tempoEstadoCandidato = millis();
        }
        else if (candidato != estadoAtual && (millis() - tempoEstadoCandidato >= DEBOUNCE_ESTADO_MS))
        {
            estadoAtual = candidato;
        }
    }

    if (estadoAtual != estadoAnterior)
    {
        String nivel = "INFO";
        if (estadoAtual == EMERGENCIA_RACK)
            nivel = "CRIT";
        else if (estadoAtual == APROXIMACAO_RACK)
            nivel = "ALERT";
        else if (estadoAtual == AVISO_SALA)
            nivel = "WARN";

        logSerial(nivel, "ESTADO", "Transicao: " + _nomeEstado(estadoAnterior) + " -> " + _nomeEstado(estadoAtual));
        enviarLogSite("Estado alterado: " + _nomeEstado(estadoAnterior) + " -> " + _nomeEstado(estadoAtual));

        if (estadoAtual == EMERGENCIA_RACK)
        {
            contadorInvasoes++;
            contadoresDirty = true;
            logSerial("CRIT", "SEGURANCA", "Invasao fisica detectada no rack!");
        }
        if (estadoAtual == APROXIMACAO_RACK)
        {
            contadorAproximacoes++;
            contadoresDirty = true;
        }

        audioPararPendente = true;
        estadoAnterior = estadoAtual;
    }
}

String Alarme::_nomeEstado(EstadoSistema e)
{
    switch (e)
    {
    case SEGURO:
        return "NORMAL";
    case ACESSO_RFID:
        return "ACESSO RFID";
    case AVISO_SALA:
        return "AVISO SALA";
    case APROXIMACAO_RACK:
        return "APROXIMACAO RACK";
    case EMERGENCIA_RACK:
        return "EMERGENCIA RACK";
    case INVASAO_SALA:
        return "INVASAO SALA";
    default:
        return "DESCONHECIDO";
    }
}