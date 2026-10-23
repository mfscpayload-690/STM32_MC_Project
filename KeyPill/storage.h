/**
 * @file storage.h
 * @brief Flash / EEPROM storage management for encrypted credentials on STM32 Blue Pill
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stddef.h>

#define STORAGE_MAGIC 0x4B50494C // "KPIL" (KeyPill identifier)

struct PasswordRecord {
    uint32_t magic;              // Validation magic number (0x4B50494C)
    uint8_t  password_len;       // Plaintext password length (e.g. 14)
    uint8_t  encrypted_len;      // Ciphertext length (16 or 32 bytes)
    uint8_t  encrypted_data[32]; // AES-128 encrypted bytes (stored ciphertext)
    uint32_t checksum;           // XOR / additive checksum for integrity check
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize flash/EEPROM storage module.
 */
void storage_init(void);

/**
 * @brief Save an encrypted password record to non-volatile flash memory.
 * @param record Pointer to populated PasswordRecord.
 * @return 1 on success, 0 on failure.
 */
int storage_save_record(const struct PasswordRecord *record);

/**
 * @brief Load an encrypted password record from non-volatile flash memory.
 * @param record Pointer to destination PasswordRecord buffer.
 * @return 1 if valid record found, 0 if empty or corrupted.
 */
int storage_load_record(struct PasswordRecord *record);

/**
 * @brief Compute a checksum for integrity validation of a PasswordRecord.
 */
uint32_t storage_compute_checksum(const struct PasswordRecord *record);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_H
