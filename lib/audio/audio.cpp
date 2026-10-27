#include "audio.h"

void Audio::begin()
{
    pinMode(Pins::BUZZER_E, OUTPUT);
    pinMode(Pins::BUZZER_D, OUTPUT);
    parar();
}

void Audio::parar()
{
    noTone(Pins::BUZZER_E);
    noTone(Pins::BUZZER_D);
}

void Audio::forcarSilencio()
{
    parar();
    _prioridadeAtual = 0;
    _tempoUltimoBip = 0;
}

void Audio::beepCurto(int freq, int dur)
{
    if (_prioridadeAtual < 3)
        tone(Pins::BUZZER_E, freq, dur);
}

void Audio::beepErro()
{
    if (_prioridadeAtual < 3)
        tone(Pins::BUZZER_D, 500, 200);
}

void Audio::beepSucesso()
{
    if (_prioridadeAtual < 3)
        tone(Pins::BUZZER_E, 800, 120);
}

void Audio::tocar(EstadoSistema estado, bool bloqueioAtivo, EstadoAmbiental ambiente)
{
    unsigned long agora = millis();

    int prioridade = 0;
    if (estado == EMERGENCIA_RACK || estado == INVASAO_SALA)
    {
        prioridade = 3;
    }
    else if (estado == AVISO_SALA || estado == APROXIMACAO_RACK || bloqueioAtivo)
    {
        prioridade = 2;
    }
    else if (ambiente == AMB_CRITICO || ambiente == AMB_ALERTA)
    {
        prioridade = 1;
    }

    if (prioridade != _prioridadeAtual)
    {
        parar();
        _tempoUltimoBip = 0;
    }
    _prioridadeAtual = prioridade;

    switch (prioridade)
    {
    case 3:
        if (estado == EMERGENCIA_RACK)
        {
            if (agora - _tempoUltimoBip >= 160)
            {
                static bool alt = false;
                tone(Pins::BUZZER_D, alt ? 900 : 1300, 140);
                alt = !alt;
                _tempoUltimoBip = agora;
            }
        }
        else if (estado == INVASAO_SALA)
        {
            if (agora - _tempoUltimoBip >= 25)
            {
                static int freq = 400, passo = 25;
                tone(Pins::BUZZER_D, freq, 20);
                freq += passo;
                if (freq >= 900 || freq <= 400)
                    passo = -passo;
                _tempoUltimoBip = agora;
            }
        }
        break;

    case 2:
        if (bloqueioAtivo)
        {
            if (agora - _tempoUltimoBip >= 800)
            {
                tone(Pins::BUZZER_D, 500, 100);
                _tempoUltimoBip = agora;
            }
        }
        else if (estado == AVISO_SALA)
        {
            if (agora - _tempoUltimoBip >= 700)
            {
                tone(Pins::BUZZER_E, 523, 100);
                _tempoUltimoBip = agora;
            }
        }
        else if (estado == APROXIMACAO_RACK)
        {
            if (agora - _tempoUltimoBip >= 400)
            {
                tone(Pins::BUZZER_E, 750, 80);
                _tempoUltimoBip = agora;
            }
        }
        break;

    case 1:
        if (ambiente == AMB_CRITICO)
        {
            if (agora - _tempoUltimoBip >= 3000)
            {
                tone(Pins::BUZZER_D, 400, 200);
                _tempoUltimoBip = agora;
            }
        }
        else if (ambiente == AMB_ALERTA)
        {
            if (agora - _tempoUltimoBip >= 8000)
            {
                tone(Pins::BUZZER_E, 700, 150);
                _tempoUltimoBip = agora;
            }
        }
        break;

    case 0:
        parar();
        break;
    }
}