#include "botao.h"

void Botao::begin()
{
    pinMode(Pins::BOTAO, INPUT_PULLUP);
    _brutoAnterior = false;
    _filtrado = false;
    _tempoUltimaMudanca = 0;
    _tempoPressionado = 0;
    _cliques = 0;
    _tempoUltimoClique = 0;
    _tempoUltimaAcao = 0;
}

EventoBotao Botao::update()
{
    bool bruto = (digitalRead(Pins::BOTAO) == LOW);

    if (bruto != _brutoAnterior)
    {
        _tempoUltimaMudanca = millis();
        _brutoAnterior = bruto;
    }
    if ((millis() - _tempoUltimaMudanca) < DEBOUNCE_BOTAO_MS)
        return BOTAO_NENHUM;
    if (bruto == _filtrado)
        return BOTAO_NENHUM;

    _filtrado = bruto;

    if (_filtrado)
    {
        _tempoPressionado = millis();
        return BOTAO_NENHUM;
    }

    unsigned long duracao = millis() - _tempoPressionado;
    if (millis() - _tempoUltimaAcao < DEBOUNCE_BOTAO_MS)
        return BOTAO_NENHUM;

    if (duracao >= DURACAO_CLIQUE_LONGO_MS)
    {
        _tempoUltimaAcao = millis();
        _cliques = 0;
        return BOTAO_LONGO;
    }

    if (duracao >= 50)
    {
        _cliques++;
        _tempoUltimoClique = millis();
    }

    if (_cliques >= 2 && (millis() - _tempoUltimoClique < JANELA_CLIQUE_DUPLO_MS))
    {
        _tempoUltimaAcao = millis();
        _cliques = 0;
        return BOTAO_DUPLO;
    }

    if (_cliques == 1 && (millis() - _tempoUltimoClique > JANELA_CLIQUE_DUPLO_MS))
    {
        _tempoUltimaAcao = millis();
        _cliques = 0;
        return BOTAO_CURTO;
    }

    return BOTAO_NENHUM;
}