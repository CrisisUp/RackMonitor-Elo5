#include <unity.h>
#include "json_builder.h"

void test_json_normal()
{
    DadosJSON d;
    d.estado = "NORMAL";
    d.modo = "ARMADO";
    d.ts = "2026-10-26T19:47:05";
    d.time_source = "RTC";
    d.porta = false;
    d.luz = false;
    d.dist_cm = 166.3;
    d.ativo = false;
    d.ip = "10.10.0.2";
    d.uid = "01020304";
    d.acessos = 5;
    d.invasoes = 1;
    d.rearmes = 1;
    d.alerta_temp = false;
    d.critico_temp = false;
    d.alerta_umid = false;
    d.critico_umid = false;
    d.taxa_temp_alerta = false;
    d.ambiente = "OK";
    d.temp_c = 18.5;
    d.umid_pct = 47.5;

    String json = gerarJSON(d);

    TEST_ASSERT_TRUE(json.indexOf("\"estado\":\"NORMAL\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"modo\":\"ARMADO\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"porta\":false") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"dist_cm\":166.3") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"temp_c\":18.5") >= 0);
    TEST_ASSERT_EQUAL('{', json[0]);
    TEST_ASSERT_EQUAL('}', json[json.length() - 1]);
}

void test_json_porta_aberta_e_sem_distancia()
{
    DadosJSON d;
    d.estado = "EMERGENCIA RACK";
    d.modo = "ARMADO";
    d.porta = true;
    d.dist_cm = -1.0;
    d.temp_c = -999.0;
    d.umid_pct = -999.0;
    d.ambiente = "CRITICO";
    d.critico_temp = true;
    d.ts = "";
    d.time_source = "";
    d.luz = false;
    d.ativo = true;
    d.ip = "";
    d.uid = "";
    d.acessos = 0;
    d.invasoes = 0;
    d.rearmes = 0;
    d.alerta_temp = false;
    d.alerta_umid = false;
    d.critico_umid = false;
    d.taxa_temp_alerta = false;

    String json = gerarJSON(d);

    TEST_ASSERT_TRUE(json.indexOf("\"dist_cm\":null") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"porta\":true") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"ativo\":true") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"critico_temp\":true") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"ambiente\":\"CRITICO\"") >= 0);
    TEST_ASSERT_TRUE(json.indexOf("\"temp_c\":-999.0") >= 0);
}

// NÃO PRECISA DE MAIN AQUI!