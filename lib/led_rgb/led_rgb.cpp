#include "led_rgb.h"

void LedRGB::begin()
{
    ledcAttach(Pins::LED_R, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(Pins::LED_G, PWM_FREQ, PWM_RESOLUTION);
    ledcAttach(Pins::LED_B, PWM_FREQ, PWM_RESOLUTION);
    definirCor(0, 0, 0);
}

void LedRGB::definirCor(int r, int g, int b)
{
    ledcWrite(Pins::LED_R, 255 - constrain(r, 0, 255));
    ledcWrite(Pins::LED_G, 255 - constrain(g, 0, 255));
    ledcWrite(Pins::LED_B, 255 - constrain(b, 0, 255));
}

void LedRGB::selfTest()
{
    definirCor(255, 0, 0);
    delay(150);
    definirCor(0, 255, 0);
    delay(150);
    definirCor(0, 0, 255);
    delay(150);
    definirCor(0, 0, 0);
}

void LedRGB::atualizar(ModoVigilante modo, EstadoSistema estado,
                       bool bloqueioAtivo, unsigned long &tempoUltimoPisca,
                       bool &estadoPisca)
{
    unsigned long agora = millis();

    if (bloqueioAtivo)
    {
        if ((agora / 250) % 2 == 0)
            definirCor(255, 200, 0);
        else
            definirCor(0, 0, 0);
        return;
    }

    if (modo == DESARMADO)
    {
        float fase = (agora % 2000) / 2000.0f;
        int brilho = (int)(127.5f * (1.0f + sin(fase * 2.0f * PI)));
        definirCor(0, 0, brilho);
        return;
    }

    switch (estado)
    {
    case SEGURO:
    case ACESSO_RFID:
        definirCor(0, 255, 0);
        break;
    case AVISO_SALA:
        definirCor(255, 200, 0);
        break;
    case APROXIMACAO_RACK:
        definirCor(255, 100, 0);
        break;
    case INVASAO_SALA:
        if (agora - tempoUltimoPisca >= 250)
        {
            estadoPisca = !estadoPisca;
            tempoUltimoPisca = agora;
        }
        definirCor(estadoPisca ? 255 : 0, 0, 0);
        break;
    case EMERGENCIA_RACK:
        definirCor(255, 0, 0);
        break;
    }
}