/**
 * @file storage.cpp
 * @brief Flash / EEPROM storage implementation for STM32 Blue Pill
 */

#include "storage.h"
#include <string.h>

#if defined(ARDUINO)
#include <Arduino.h>
#include <EEPROM.h>
#else
// Mock storage for host-side unit testing
static uint8_t mock_eeprom[sizeof(struct PasswordRecord)];
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

int storage_save_record(const struct PasswordRecord *record) {
    if (!record) return 0;

    struct PasswordRecord rec_to_save;
    memcpy(&rec_to_save, record, sizeof(struct PasswordRecord));
    rec_to_save.magic = STORAGE_MAGIC;
    rec_to_save.checksum = storage_compute_checksum(&rec_to_save);

#if defined(ARDUINO)
    const uint8_t *ptr = (const uint8_t *)&rec_to_save;
    for (size_t i = 0; i < sizeof(struct PasswordRecord); ++i) {
        EEPROM.write(i, ptr[i]);
    }
    #if defined(ARDUINO_ARCH_STM32) && defined(EEPROM_FORMAT)
    // Some cores support commit/format
    #endif
    return 1;
#else
    memcpy(mock_eeprom, &rec_to_save, sizeof(struct PasswordRecord));
    return 1;
#endif
}

int storage_load_record(struct PasswordRecord *record) {
    if (!record) return 0;

    struct PasswordRecord loaded;
#if defined(ARDUINO)
    uint8_t *ptr = (uint8_t *)&loaded;
    for (size_t i = 0; i < sizeof(struct PasswordRecord); ++i) {
        ptr[i] = EEPROM.read(i);
    }
#else
    memcpy(&loaded, mock_eeprom, sizeof(struct PasswordRecord));
#endif

    // Validate magic header
    if (loaded.magic != STORAGE_MAGIC) {
        return 0; // Empty or uninitialized flash
    }

    // Validate checksum
    uint32_t expected_sum = storage_compute_checksum(&loaded);
    if (loaded.checksum != expected_sum) {
        return 0; // Data corrupted
    }

    memcpy(record, &loaded, sizeof(struct PasswordRecord));
    return 1;
}
