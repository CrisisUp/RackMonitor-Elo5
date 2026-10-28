#include "hmac.h"
#include <mbedtls/md.h>

/**
 * @brief Gera uma chave HMAC aleatória de 32 caracteres e salva na NVS.
 */
void gerarChaveHMAC(Preferences &prefs)
{
    String chave = "";
    for (int i = 0; i < 32; i++)
    {
        chave += String((char)random(33, 126)); // ASCII imprimível
    }
    prefs.begin("rack", false);
    prefs.putString("hmac_key", chave);
    prefs.end();
    Serial.println(F("[HMAC] Chave gerada e salva na NVS:"));
    Serial.println(chave);
    Serial.println(F("[HMAC] Anote esta chave! Ela não será exibida novamente."));
}

/**
 * @brief Obtém a chave HMAC da NVS.
 */
String obterChaveHMAC(Preferences &prefs)
{
    prefs.begin("rack", true);
    String chave = prefs.getString("hmac_key", "");
    prefs.end();
    return chave;
}

/**
 * @brief Calcula o HMAC-SHA256 de um payload usando a chave.
 */
String calcularHMAC(const String &payload, const String &chave)
{
    byte hmac[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, (const byte *)chave.c_str(), chave.length());
    mbedtls_md_hmac_update(&ctx, (const byte *)payload.c_str(), payload.length());
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    String sig = "";
    for (int i = 0; i < 32; i++)
    {
        if (hmac[i] < 0x10)
            sig += "0";
        sig += String(hmac[i], HEX);
    }
    sig.toUpperCase();
    return sig;
}

/**
 * @brief Valida um comando HMAC com anti-replay.
 */
bool validarHMAC(const String &cmd, uint32_t ts, const String &sig, Preferences &prefs, uint32_t agora)
{
    // 1. Anti-replay: timestamp dentro de ±60s
    long diff = (long)agora - (long)ts;
    if (diff > 60 || diff < -60)
    {
        Serial.print(F("[HMAC] Timestamp fora da janela. Agora: "));
        Serial.print(agora);
        Serial.print(F(" | Recebido: "));
        Serial.println(ts);
        return false;
    }

    // 2. Obter chave
    String chave = obterChaveHMAC(prefs);
    if (chave.length() == 0)
    {
        Serial.println(F("[HMAC] Chave nao encontrada na NVS."));
        return false;
    }

    // 3. Montar payload e calcular HMAC esperado
    String payload = cmd + "|" + String(ts);

    // --- LOGS DE DIAGNOSTICO (remover depois) ---
    Serial.print(F("[HMAC] Payload: "));
    Serial.println(payload);
    Serial.print(F("[HMAC] Chave (primeiros 8): "));
    Serial.println(chave.substring(0, 8));
    // --------------------------------------------

    String sigEsperada = calcularHMAC(payload, chave);

    // 4. Comparar (tempo constante)
    if (sigEsperada.length() != sig.length())
    {
        Serial.print(F("[HMAC] Tamanho de assinatura invalido. Esperada: "));
        Serial.print(sigEsperada);
        Serial.print(F(" | Recebida: "));
        Serial.println(sig);
        return false;
    }

    byte diffByte = 0;
    for (size_t i = 0; i < sigEsperada.length(); i++)
    {
        diffByte |= (sigEsperada[i] ^ sig[i]);
    }

    if (diffByte != 0)
    {
        Serial.print(F("[HMAC] Assinatura invalida. Esperada: "));
        Serial.print(sigEsperada);
        Serial.print(F(" | Recebida: "));
        Serial.println(sig);
        return false;
    }

    return true;
}