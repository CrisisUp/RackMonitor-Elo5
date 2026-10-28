#include "nvs.h"
#include "../../include/config.h"
#include "../../include/state.h"

void salvarConfig()
{
    prefs.begin("rack", false);
    prefs.putFloat("limiar", alarme.limiarDistancia);
    prefs.putULong("timeout", alarme.timeoutDesarme);
    prefs.putUInt("invasoes", alarme.contadorInvasoes);
    prefs.putUInt("aberturas", alarme.contadorAberturasPorta);
    prefs.putUInt("acessos", alarme.contadorAcessosValidos);
    prefs.putUInt("desarmes", alarme.contadorDesarmes);
    prefs.putUInt("rearmes", alarme.contadorRearmesBloqueados);
    prefs.putFloat("t_alert", limiarTempAlerta);
    prefs.putFloat("t_crit", limiarTempCritico);
    prefs.putFloat("u_alert", limiarUmidAlerta);
    prefs.putFloat("u_crit", limiarUmidCritico);
    prefs.end();
    logSerial("INFO", "NVS", "Configuracoes salvas na flash.");
}

void carregarCartoes()
{
    cartoesAutorizados.clear();

    auto adicionarSeNovo = [](const String &uid)
    {
        for (const String &c : cartoesAutorizados)
            if (c == uid)
                return;
        cartoesAutorizados.push_back(uid);
    };

    prefs.begin("rack", true);

    adicionarSeNovo(CARTAO_AUTORIZADO_1);
    adicionarSeNovo(CARTAO_AUTORIZADO_2);

    int total = prefs.getInt("total_cards", 0);
    for (int i = 0; i < total; i++)
    {
        String chave = "card_" + String(i);
        String uid = prefs.getString(chave.c_str(), "");
        if (uid.length() > 0)
        {
            adicionarSeNovo(uid);
            logSerial("INFO", "NVS", "Cartao carregado: " + uid);
        }
    }
    prefs.end();
    logSerial("INFO", "NVS", "Total de cartoes carregados: " + String(cartoesAutorizados.size()));
}

void carregarConfig()
{
    prefs.begin("rack", true);
    alarme.limiarDistancia = prefs.getFloat("limiar", 50.0f);
    alarme.timeoutDesarme = prefs.getULong("timeout", 30UL * 60UL * 1000UL);
    alarme.contadorInvasoes = prefs.getUInt("invasoes", 0);
    alarme.contadorAberturasPorta = prefs.getUInt("aberturas", 0);
    alarme.contadorAcessosValidos = prefs.getUInt("acessos", 0);
    alarme.contadorDesarmes = prefs.getUInt("desarmes", 0);
    alarme.contadorRearmesBloqueados = prefs.getUInt("rearmes", 0);
    limiarTempAlerta = prefs.getFloat("t_alert", TEMP_ALERTA_C_DEFAULT);
    limiarTempCritico = prefs.getFloat("t_crit", TEMP_CRITICO_C_DEFAULT);
    limiarUmidAlerta = prefs.getFloat("u_alert", UMID_ALERTA_PCT_DEFAULT);
    limiarUmidCritico = prefs.getFloat("u_crit", UMID_CRITICO_PCT_DEFAULT);
    uint8_t modoSalvo = prefs.getUInt("modo", (uint8_t)ARMADO);
    alarme.modo = (modoSalvo == (uint8_t)DESARMADO) ? DESARMADO : ARMADO;
    prefs.end();
    logSerial("INFO", "NVS", "Configuracoes carregadas da flash.");
    logSerial("INFO", "NVS", "Modo restaurado: " + String(alarme.modo == ARMADO ? "ARMADO" : "DESARMADO"));
}