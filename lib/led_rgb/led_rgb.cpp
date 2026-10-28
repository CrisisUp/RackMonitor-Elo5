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
    // Catodo comum: valor direto (0 = apagado, 255 = brilho máximo)
    ledcWrite(Pins::LED_R, constrain(r, 0, 255));
    ledcWrite(Pins::LED_G, constrain(g, 0, 255));
    ledcWrite(Pins::LED_B, constrain(b, 0, 255));
}

void LedRGB::selfTest()
{
    definirCor(255, 0, 0); // vermelho
    delay(150);
    definirCor(0, 255, 0); // verde
    delay(150);
    definirCor(0, 0, 255); // azul
    delay(150);
    definirCor(0, 0, 0); // apagado
}

void LedRGB::atualizar(ModoVigilante modo, EstadoSistema estado,
                       bool bloqueioAtivo, unsigned long &tempoUltimoPisca,
                       bool &estadoPisca)
{
    unsigned long agora = millis();

    if (bloqueioAtivo)
    {
        if ((agora / 250) % 2 == 0)
            definirCor(255, 200, 0); // amarelo piscando
        else
            definirCor(0, 0, 0);
        return;
    }

    if (modo == DESARMADO)
    {
        // Azul pulsante (respiração)
        float fase = (agora % 2000) / 2000.0f;
        int brilho = (int)(127.5f * (1.0f + sin(fase * 2.0f * PI)));
        definirCor(0, 0, brilho);
        return;
    }

    switch (estado)
    {
    case SEGURO:
    case ACESSO_RFID:
        definirCor(0, 255, 0); // verde (tudo OK)
        break;
    case AVISO_SALA:
        definirCor(255, 200, 0); // amarelo
        break;
    case APROXIMACAO_RACK:
        definirCor(255, 100, 0); // laranja
        break;
    case INVASAO_SALA:
        if (agora - tempoUltimoPisca >= 250)
        {
            estadoPisca = !estadoPisca;
            tempoUltimoPisca = agora;
        }
        definirCor(estadoPisca ? 255 : 0, 0, 0); // vermelho piscando
        break;
    case EMERGENCIA_RACK:
        definirCor(255, 0, 0); // vermelho fixo
        break;
    }
}