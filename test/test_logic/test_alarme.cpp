#include <unity.h>

// =========================================================================
// SIMULAÇÃO DA LÓGICA DO ALARME (para testes nativos)
// =========================================================================
// Como o Alarme real depende de hardware, usamos uma função pura que replica
// exatamente a lógica de prioridade do _atualizarEstados() no main.cpp.
// =========================================================================

enum EstadoSistema
{
    SEGURO,
    AVISO_SALA,
    APROXIMACAO_RACK,
    EMERGENCIA_RACK,
    INVASAO_SALA,
    ACESSO_RFID
};

// Função pura que replica a lógica de transição
EstadoSistema calcularEstado(bool portaAberta, bool movimento, bool rfidRecente, bool aproximacao)
{
    if (portaAberta)
        return EMERGENCIA_RACK;
    if (rfidRecente)
        return ACESSO_RFID;
    if (movimento)
        return AVISO_SALA;
    if (aproximacao)
        return APROXIMACAO_RACK;
    return SEGURO;
}

// =========================================================================
// TESTES
// =========================================================================

void test_porta_aberta_vence_rfid(void)
{
    // Porta aberta + RFID recente = EMERGENCIA (porta vence)
    EstadoSistema e = calcularEstado(true, true, true, false);
    TEST_ASSERT_EQUAL(EMERGENCIA_RACK, e);
}

void test_rfid_acesso_normal(void)
{
    // Sem porta aberta, mas com RFID = ACESSO_RFID
    EstadoSistema e = calcularEstado(false, false, true, false);
    TEST_ASSERT_EQUAL(ACESSO_RFID, e);
}

void test_movimento_aviso(void)
{
    // Só movimento = AVISO_SALA
    EstadoSistema e = calcularEstado(false, true, false, false);
    TEST_ASSERT_EQUAL(AVISO_SALA, e);
}

void test_tudo_normal(void)
{
    // Nada ativo = SEGURO
    EstadoSistema e = calcularEstado(false, false, false, false);
    TEST_ASSERT_EQUAL(SEGURO, e);
}