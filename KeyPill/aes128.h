/**
 * @file aes128.h
 * @brief Standalone, self-contained AES-128 Encryption & Decryption (NIST FIPS 197)
 *        Zero external library dependencies - directly portable to Arduino / STM32.
 */

#ifndef AES128_H
#define AES128_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AES_BLOCKLEN 16 // Block length in bytes (128 bits)
#define AES_KEYLEN   16 // Key length in bytes (128 bits)

/**
 * @brief Encrypts a single 16-byte block using AES-128 (ECB mode).
 * @param input Pointer to 16 bytes of plaintext data.
 * @param key Pointer to 16-byte encryption key.
 * @param output Pointer to 16-byte buffer for resulting ciphertext.
 */
void aes128_encrypt_block(const uint8_t *input, const uint8_t *key, uint8_t *output);

/**
 * @brief Decrypts a single 16-byte block using AES-128 (ECB mode).
 * @param input Pointer to 16 bytes of ciphertext data.
 * @param key Pointer to 16-byte decryption key.
 * @param output Pointer to 16-byte buffer for resulting plaintext.
 */
void aes128_decrypt_block(const uint8_t *input, const uint8_t *key, uint8_t *output);

/**
 * @brief Encrypts arbitrary buffer with PKCS#7 padding using AES-128.
 * @param input Input plaintext buffer.
 * @param length Input plaintext length in bytes (1 to 16 bytes).
 * @param key 16-byte encryption key.
 * @param output Buffer receiving ciphertext (must be at least 16 or 32 bytes).
 * @param out_length Pointer receiving the padded encrypted length (16 or 32).
 */
void aes128_encrypt_padded(const uint8_t *input, size_t length, const uint8_t *key, uint8_t *output, size_t *out_length);

/**
 * @brief Decrypts ciphertext and strips PKCS#7 padding.
 * @param input Input ciphertext buffer.
 * @param length Ciphertext length (must be multiple of 16).
 * @param key 16-byte decryption key.
 * @param output Buffer receiving decrypted plaintext (at least length bytes).
 * @param out_length Pointer receiving the original unpadded length.
 * @return 1 on success, 0 on invalid padding or corruption.
 */
int aes128_decrypt_padded(const uint8_t *input, size_t length, const uint8_t *key, uint8_t *output, size_t *out_length);

#ifdef __cplusplus
}
#endif

#endif // AES128_H
