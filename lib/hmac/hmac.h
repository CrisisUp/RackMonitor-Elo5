#pragma once
#include <Arduino.h>
#include <Preferences.h>

/**
 * @brief Gera uma chave HMAC aleatória de 32 caracteres e salva na NVS.
 *        Deve ser chamado apenas se a chave ainda não existir.
 * @param prefs Referência ao Preferences.
 */
void gerarChaveHMAC(Preferences &prefs);

/**
 * @brief Obtém a chave HMAC da NVS.
 * @param prefs Referência ao Preferences.
 * @return A chave, ou string vazia se não existir.
 */
String obterChaveHMAC(Preferences &prefs);

/**
 * @brief Calcula o HMAC-SHA256 de um payload usando a chave.
 * @param payload String no formato "COMANDO|TS".
 * @param chave Chave secreta.
 * @return String em hexadecimal (64 caracteres).
 */
String calcularHMAC(const String &payload, const String &chave);

/**
 * @brief Valida um comando HMAC com anti-replay.
 * @param cmd  Comando (ex: "ARMAR")
 * @param ts   Timestamp Unix (segundos) recebido no payload.
 * @param sig  Assinatura HMAC recebida (hex, 64 chars).
 * @param prefs Referência ao Preferences.
 * @param agora Timestamp Unix atual (fornecido pelo chamador, ex: relogio).
 * @return true se válido, false caso contrário.
 */
bool validarHMAC(const String &cmd, uint32_t ts, const String &sig, Preferences &prefs, uint32_t agora);