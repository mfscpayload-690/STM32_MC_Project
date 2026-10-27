/**
 * @file storage.cpp
 * @brief Flash / EEPROM storage implementation for STM32 Blue Pill
 */

#include "storage.h"
#include <string.h>

#if defined(ARDUINO)
#ifndef GCC_VERSION
#define GCC_VERSION 140201
#endif
#include <Arduino.h>
#include <EEPROM.h>
#else
// Mock storage for host-side unit testing: 10 slots
static uint8_t mock_eeprom[sizeof(struct PasswordRecord) * STORAGE_NUM_SLOTS];
#endif

uint32_t storage_compute_checksum(const struct PasswordRecord *record) {
    if (!record) return 0;
    uint32_t sum = record->magic ^ ((uint32_t)record->password_len << 8) ^ ((uint32_t)record->encrypted_len << 16);
    for (size_t i = 0; i < sizeof(record->encrypted_data); ++i) {
        sum = (sum * 31) + record->encrypted_data[i];
    }
    return sum;
}

void storage_init(void) {
#if defined(ARDUINO)
    #if defined(EEPROM_START_ADDRESS) || defined(EEPROM_PAGE_SIZE)
    // STM32duino internal flash-backed EEPROM
    EEPROM.init();
    #endif
#else
    memset(mock_eeprom, 0, sizeof(mock_eeprom));
#endif
}

int storage_save_slot(uint8_t slot, const struct PasswordRecord *record) {
    if (slot >= STORAGE_NUM_SLOTS || !record) return 0;

    struct PasswordRecord rec_to_save;
    memcpy(&rec_to_save, record, sizeof(struct PasswordRecord));
    rec_to_save.magic = STORAGE_MAGIC;
    rec_to_save.checksum = storage_compute_checksum(&rec_to_save);

    uint16_t base_addr = (uint16_t)slot * sizeof(struct PasswordRecord);
    const uint8_t *ptr = (const uint8_t *)&rec_to_save;

#if defined(ARDUINO)
    for (size_t i = 0; i < sizeof(struct PasswordRecord); ++i) {
        EEPROM.write(base_addr + i, ptr[i]);
    }
    return 1;
#else
    memcpy(&mock_eeprom[base_addr], ptr, sizeof(struct PasswordRecord));
    return 1;
#endif
}

int storage_load_slot(uint8_t slot, struct PasswordRecord *record) {
    if (slot >= STORAGE_NUM_SLOTS || !record) return 0;

    uint16_t base_addr = (uint16_t)slot * sizeof(struct PasswordRecord);
    struct PasswordRecord loaded;

#if defined(ARDUINO)
    uint8_t *ptr = (uint8_t *)&loaded;
    for (size_t i = 0; i < sizeof(struct PasswordRecord); ++i) {
        ptr[i] = EEPROM.read(base_addr + i);
    }
#else
    memcpy(&loaded, &mock_eeprom[base_addr], sizeof(struct PasswordRecord));
#endif

    // Validate magic header
    if (loaded.magic != STORAGE_MAGIC) {
        return 0; // Empty or uninitialized slot
    }

    // Validate checksum
    uint32_t expected_sum = storage_compute_checksum(&loaded);
    if (loaded.checksum != expected_sum) {
        return 0; // Data corrupted
    }

    memcpy(record, &loaded, sizeof(struct PasswordRecord));
    return 1;
}

int storage_clear_slot(uint8_t slot) {
    if (slot >= STORAGE_NUM_SLOTS) return 0;
    uint16_t base_addr = (uint16_t)slot * sizeof(struct PasswordRecord);

#if defined(ARDUINO)
    for (size_t i = 0; i < sizeof(struct PasswordRecord); ++i) {
        EEPROM.write(base_addr + i, 0xFF);
    }
    return 1;
#else
    memset(&mock_eeprom[base_addr], 0xFF, sizeof(struct PasswordRecord));
    return 1;
#endif
}

int storage_is_slot_occupied(uint8_t slot) {
    struct PasswordRecord rec;
    return storage_load_slot(slot, &rec);
}

int storage_find_first_empty_slot(void) {
    for (uint8_t s = 0; s < STORAGE_NUM_SLOTS; ++s) {
        if (!storage_is_slot_occupied(s)) {
            return (int)s; // First unoccupied slot (0 to 9)
        }
    }
    return -1; // All 10 slots are occupied
}

uint8_t storage_count_occupied(void) {
    uint8_t count = 0;
    for (uint8_t s = 0; s < STORAGE_NUM_SLOTS; ++s) {
        if (storage_is_slot_occupied(s)) {
            count++;
        }
    }
    return count;
}

int storage_save_record(const struct PasswordRecord *record) {
    return storage_save_slot(0, record);
}

int storage_load_record(struct PasswordRecord *record) {
    return storage_load_slot(0, record);
}
