#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cstdio>

// =========================================================================
// STUBS DO ARDUINO PARA O AMBIENTE NATIVE
// =========================================================================
// O RingBufferLog usa funções do Arduino. Como estamos testando no PC,
// precisamos simular essas funções para que o código compile.

// Simula o millis() do Arduino (retorna um contador crescente)
static unsigned long _simulated_millis = 0;
unsigned long millis()
{
    _simulated_millis += 10; // Avança 10ms a cada chamada
    return _simulated_millis;
}

// Simula as interrupções (no PC, não há interrupções reais)
void noInterrupts() {}
void interrupts() {}

// =========================================================================
// COPIA DO RING BUFFER (para testar isoladamente)
// =========================================================================
// Como o RingBufferLog original está dentro do main.cpp (que tem hardware),
// copiamos a lógica pura para cá. Se você mudar a lógica no main.cpp,
// mude aqui também (ou mova a classe para um header compartilhado).

constexpr uint16_t HIST_TAMANHO = 100;
constexpr uint8_t HIST_MSG_MAX = 64;

enum NivelLog : uint8_t
{
    NIVEL_INFO = 0,
    NIVEL_WARN = 1,
    NIVEL_ALERT = 2,
    NIVEL_CRIT = 3
};

struct EventoLog
{
    uint32_t ms;
    uint8_t nivel;
    char msg[HIST_MSG_MAX];
};

class RingBufferLog
{
public:
    void begin()
    {
        _head = 0;
        _count = 0;
    }

    void push(uint8_t nivel, const char *msg)
    {
        noInterrupts();
        EventoLog &slot = _buffer[_head];
        slot.ms = millis();
        slot.nivel = nivel;
        strncpy(slot.msg, msg, HIST_MSG_MAX - 1);
        slot.msg[HIST_MSG_MAX - 1] = '\0';

        _head = (_head + 1) % HIST_TAMANHO;
        if (_count < HIST_TAMANHO)
            _count++;
        interrupts();
    }

    void clear()
    {
        noInterrupts();
        _head = 0;
        _count = 0;
        interrupts();
    }

    uint16_t tamanho() const { return _count; }
    uint16_t head() const { return _head; }

    // Método para acessar um evento (útil para testes)
    const EventoLog *getEvento(uint16_t index) const
    {
        if (index >= _count)
            return nullptr;
        uint16_t inicio = (_head + HIST_TAMANHO - _count) % HIST_TAMANHO;
        uint16_t idx = (inicio + index) % HIST_TAMANHO;
        return &_buffer[idx];
    }

private:
    EventoLog _buffer[HIST_TAMANHO];
    volatile uint16_t _head = 0;
    volatile uint16_t _count = 0;
};

// =========================================================================
// TESTES
// =========================================================================

void test_ringbuffer_inicialmente_vazio(void)
{
    RingBufferLog buffer;
    buffer.begin();

    TEST_ASSERT_EQUAL(0, buffer.tamanho());
    TEST_ASSERT_EQUAL(0, buffer.head());
    TEST_ASSERT_NULL(buffer.getEvento(0));
}

void test_ringbuffer_push_e_tamanho(void)
{
    RingBufferLog buffer;
    buffer.begin();

    buffer.push(NIVEL_INFO, "Evento 1");
    TEST_ASSERT_EQUAL(1, buffer.tamanho());

    buffer.push(NIVEL_WARN, "Evento 2");
    TEST_ASSERT_EQUAL(2, buffer.tamanho());

    buffer.push(NIVEL_CRIT, "Evento 3");
    TEST_ASSERT_EQUAL(3, buffer.tamanho());
}

void test_ringbuffer_conteudo(void)
{
    RingBufferLog buffer;
    buffer.begin();

    buffer.push(NIVEL_INFO, "Teste de conteudo");

    const EventoLog *ev = buffer.getEvento(0);
    TEST_ASSERT_NOT_NULL(ev);
    TEST_ASSERT_EQUAL(NIVEL_INFO, ev->nivel);
    TEST_ASSERT_EQUAL_STRING("Teste de conteudo", ev->msg);
}

void test_ringbuffer_overflow(void)
{
    RingBufferLog buffer;
    buffer.begin();

    // Enche o buffer completamente
    char msg[32];
    for (int i = 0; i < HIST_TAMANHO; i++)
    {
        snprintf(msg, sizeof(msg), "Evento %d", i);
        buffer.push(NIVEL_INFO, msg);
    }

    TEST_ASSERT_EQUAL(HIST_TAMANHO, buffer.tamanho());

    // Adiciona mais um (deve sobrescrever o mais antigo)
    buffer.push(NIVEL_CRIT, "Overflow");
    TEST_ASSERT_EQUAL(HIST_TAMANHO, buffer.tamanho());

    // O último evento deve ser o de overflow
    const EventoLog *ultimo = buffer.getEvento(HIST_TAMANHO - 1);
    TEST_ASSERT_NOT_NULL(ultimo);
    TEST_ASSERT_EQUAL_STRING("Overflow", ultimo->msg);

    // O primeiro evento (índice 0) agora deve ser o "Evento 1" (o "Evento 0" foi sobrescrito)
    const EventoLog *primeiro = buffer.getEvento(0);
    TEST_ASSERT_NOT_NULL(primeiro);
    TEST_ASSERT_EQUAL_STRING("Evento 1", primeiro->msg);
}

void test_ringbuffer_clear(void)
{
    RingBufferLog buffer;
    buffer.begin();

    buffer.push(NIVEL_INFO, "Evento 1");
    buffer.push(NIVEL_WARN, "Evento 2");
    TEST_ASSERT_EQUAL(2, buffer.tamanho());

    buffer.clear();
    TEST_ASSERT_EQUAL(0, buffer.tamanho());
    TEST_ASSERT_EQUAL(0, buffer.head());
}

void test_ringbuffer_mensagem_longa_truncada(void)
{
    RingBufferLog buffer;
    buffer.begin();

    // Cria uma string maior que HIST_MSG_MAX (64)
    char msgLonga[100];
    memset(msgLonga, 'A', 99);
    msgLonga[99] = '\0';

    buffer.push(NIVEL_INFO, msgLonga);

    const EventoLog *ev = buffer.getEvento(0);
    TEST_ASSERT_NOT_NULL(ev);
    // A string deve ter sido truncada para HIST_MSG_MAX - 1 (63 caracteres)
    TEST_ASSERT_EQUAL(HIST_MSG_MAX - 1, strlen(ev->msg));
}