/**
 * @file storage.h
 * @brief Flash / EEPROM storage management for encrypted credentials on STM32 Blue Pill
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include <stddef.h>

#define STORAGE_MAGIC 0x4B50494C // "KPIL" (KeyPill identifier)

#define STORAGE_NUM_SLOTS 10 // 10 distinct non-volatile password slots

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
 * @brief Multi-slot storage operations (slot: 0 to STORAGE_NUM_SLOTS - 1).
 */
int storage_save_slot(uint8_t slot, const struct PasswordRecord *record);
int storage_load_slot(uint8_t slot, struct PasswordRecord *record);
int storage_clear_slot(uint8_t slot);
int storage_is_slot_occupied(uint8_t slot);
int storage_find_first_empty_slot(void); // Returns 0..9, or -1 if full (10/10)
uint8_t storage_count_occupied(void);

/**
 * @brief Backward-compatibility single-record functions (maps to slot 0).
 */
int storage_save_record(const struct PasswordRecord *record);
int storage_load_record(struct PasswordRecord *record);

/**
 * @brief Compute a checksum for integrity validation of a PasswordRecord.
 */
uint32_t storage_compute_checksum(const struct PasswordRecord *record);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_H
