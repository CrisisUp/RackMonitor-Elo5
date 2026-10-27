#include <unity.h>

// Declaração das funções de teste (Alarme)
void test_porta_aberta_vence_rfid(void);
void test_rfid_acesso_normal(void);
void test_movimento_aviso(void);
void test_tudo_normal(void);

// Declaração das funções de teste (JSON)
void test_json_normal(void);
void test_json_porta_aberta_e_sem_distancia(void);

// Declaração das funções de teste (RingBuffer)
void test_ringbuffer_inicialmente_vazio(void);
void test_ringbuffer_push_e_tamanho(void);
void test_ringbuffer_conteudo(void);
void test_ringbuffer_overflow(void);
void test_ringbuffer_clear(void);
void test_ringbuffer_mensagem_longa_truncada(void);

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    // Testes do Alarme
    RUN_TEST(test_porta_aberta_vence_rfid);
    RUN_TEST(test_rfid_acesso_normal);
    RUN_TEST(test_movimento_aviso);
    RUN_TEST(test_tudo_normal);

    // Testes do JSON
    RUN_TEST(test_json_normal);
    RUN_TEST(test_json_porta_aberta_e_sem_distancia);

    // Testes do RingBuffer
    RUN_TEST(test_ringbuffer_inicialmente_vazio);
    RUN_TEST(test_ringbuffer_push_e_tamanho);
    RUN_TEST(test_ringbuffer_conteudo);
    RUN_TEST(test_ringbuffer_overflow);
    RUN_TEST(test_ringbuffer_clear);
    RUN_TEST(test_ringbuffer_mensagem_longa_truncada);

    return UNITY_END();
}