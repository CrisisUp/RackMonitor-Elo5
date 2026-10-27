#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "../../include/config.h"
#include "../../include/state.h"
#include "../audio/audio.h"
#include "../../include/ring_buffer.h"
#include "../porta/porta.h"

// Declara que essas variáveis/funções existem no main.cpp
extern Preferences prefs;
extern Audio audio;
extern RingBufferLog logBuffer;
extern bool contadoresDirty;
extern std::vector<String> cartoesAutorizados;
void logSerial(const String &nivel, const String &tag, const String &mensagem);
void enviarLogSite(const String &mensagem);

class Alarme
{
public:
    ModoVigilante modo = ARMADO;
    EstadoSistema estadoAtual = SEGURO;
    EstadoSistema estadoAnterior = SEGURO;
    EstadoSistema estadoCandidato = SEGURO;
    unsigned long tempoEstadoCandidato = 0;

    unsigned int contadorDesarmes = 0;
    unsigned int contadorRearmesBloqueados = 0;
    unsigned int contadorAberturasPorta = 0;
    unsigned int contadorInvasoes = 0;
    unsigned int contadorAproximacoes = 0;
    unsigned int contadorAcessosValidos = 0;
    unsigned int contadorMovimentosManutencao = 0;

    unsigned long timeoutDesarme = 30UL * 60UL * 1000UL;
    unsigned long tempoInicioDesarme = 0;
    bool aviso5minDisparado = false;
    String uidUltimoDesarme = "";
    String uidUltimoRearme = "";
    bool ultimoCartaoValido = false;
    unsigned long duracaoUltimoDesarme = 0;
    unsigned long ultimoUIDtempoRFID = 0;

    String motivoBloqueio = "";
    unsigned long tempoBloqueioAtivo = 0;

    bool resetContadoresPendente = false;
    bool modoTestePendente = false;
    bool audioPararPendente = false;

    float limiarDistancia = 50.0f;

    void update(const LeiturasSensores &s, EventoRFID rfidEv, EventoBotao botaoEv,
                Porta &porta, Audio &audio);
    bool bloqueioAtivo() const;
    void dispararPanico(const String &origem);
    bool armarViaSerial(Porta &porta, Audio &audio);
    String nomeModo() const;
    String nomeEstadoAtual() const;

private:
    bool _estadoAnteriorPIR = false;
    bool _estadoAnteriorLDR = false;
    bool _portaEstadoAnterior = false;
    bool _primeiraLeitura = true;

    void _logBordas(const LeiturasSensores &s);
    void _logPorta(Porta &porta);
    void _tratarRFID(const EventoRFID &ev, Porta &porta, Audio &audio);
    void _tratarBotao(EventoBotao ev, Porta &porta, Audio &audio);
    void _alternarModo(const String &uid, Porta &porta, Audio &audio);
    bool _tentarArmar(Porta &porta, Audio &audio);
    void _rearmarPorTimeout(Porta &porta, Audio &audio);
    void _atualizarEstados(const LeiturasSensores &s, const EventoRFID &rfidEv);
    static String _nomeEstado(EstadoSistema e);
};