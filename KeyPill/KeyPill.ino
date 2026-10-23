/**
 * ==============================================================================
 * Project: KeyPill - Secure Hardware Password Generator & Auto-Filler
 * Target Hardware: STM32F103C8T6 (STM32 Blue Pill) + ST-Link V2
 * Environment: Arduino IDE with STM32duino (Official STMicroelectronics Core)
 * Author: Group 2 (Aravind Lal, Betsa Mariya Philip, Karthik V Panicker, Vaishnav Prasad A)
 * Course: Microcontrollers (PBCST504) - KTU
 * ==============================================================================
 *
 * HOW IT WORKS:
 * 1. User presses button on PA0.
 * 2. Hardware entropy (SysTick + floating ADC PA1 + STM32 Unique Chip ID) seeds PRNG.
 * 3. STM32 generates a strong random 14-character password (Upper, Lower, Number, Symbol).
 * 4. Password is encrypted using AES-128 (NIST FIPS 197).
 * 5. Encrypted ciphertext is safely stored in STM32 internal flash memory.
 * 6. Ciphertext is decrypted back into plaintext buffer in isolated SRAM.
 * 7. STM32 emulates a USB HID Keyboard and types the password directly into PC + presses ENTER.
 * 8. Plaintext memory buffers are securely zeroed out (erased).
 *
 * ARDUINO IDE SETTINGS:
 * - Board: "Generic STM32F1 series"
 * - Board part number: "BluePill F103C8"
 * - USB support: "HID (keyboard/mouse)"  <-- IMPORTANT!
 * - Upload method: "STM32CubeProgrammer (SWD)" or "STLink"
 * ==============================================================================
 */

#include <Arduino.h>
#include <Keyboard.h>
#include "aes128.h"
#include "password_gen.h"
#include "storage.h"

// ------------------------------------------------------------------------------
// Hardware Pin Definitions
// ------------------------------------------------------------------------------
#define PIN_BUTTON       PA0   // Push button input (active HIGH with 10k pull-down)
#define PIN_STATUS_LED   PC13  // Onboard Blue Pill LED (active LOW: LOW = ON, HIGH = OFF)
#define PIN_ENTROPY_ADC  PA1   // Unconnected floating analog pin for environmental noise

// ------------------------------------------------------------------------------
// Security Configuration
// ------------------------------------------------------------------------------
// 128-bit AES Master Key (16 bytes). Can be customized or salted with Hardware UID.
static const uint8_t MASTER_AES_KEY[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

#define GENERATE_PASSWORD_LENGTH 14  // 12 to 16 characters

// ------------------------------------------------------------------------------
// Helper: LED feedback routines
// ------------------------------------------------------------------------------
void set_led(bool state) {
    // Blue Pill PC13 LED is active LOW
    digitalWrite(PIN_STATUS_LED, state ? LOW : HIGH);
}

void blink_led(int times, int delay_ms) {
    for (int i = 0; i < times; ++i) {
        set_led(true);
        delay(delay_ms);
        set_led(false);
        delay(delay_ms);
    }
}

// ------------------------------------------------------------------------------
// Helper: Secure memory cleanup (prevent password retention in RAM)
// ------------------------------------------------------------------------------
void secure_zero_memory(void *v, size_t n) {
    volatile uint8_t *p = (volatile uint8_t *)v;
    while (n--) {
        *p++ = 0;
    }
}

// ------------------------------------------------------------------------------
// USB Force Re-enumeration for STM32 Blue Pill
// Cheap Blue Pill boards have a hardwired 10k/4.7k resistor on PA12 (USB D+).
// Pulling PA12 LOW momentarily simulates a USB unplug/replug event so PC detects HID.
// ------------------------------------------------------------------------------
void trigger_usb_reenumeration(void) {
    pinMode(PA12, OUTPUT);
    digitalWrite(PA12, LOW); // Force D+ low (disconnect)
    delay(150);
    pinMode(PA12, INPUT);   // Release D+ (re-attach)
    delay(150);
}

// ------------------------------------------------------------------------------
// Core Process: Generate -> Encrypt -> Store -> Decrypt -> Type via USB HID
// ------------------------------------------------------------------------------
void execute_credential_pipeline(bool generate_new) {
    set_led(true); // Turn on LED while processing

    char plaintext_password[MAX_PASSWORD_LEN + 1];
    memset(plaintext_password, 0, sizeof(plaintext_password));

    struct PasswordRecord record;
    memset(&record, 0, sizeof(record));

    if (generate_new) {
        // --- 1. HARVEST HARDWARE ENTROPY & GENERATE PASSWORD ---
        uint32_t adc_noise = analogRead(PIN_ENTROPY_ADC);
        uint32_t time_us = micros();
        uint32_t systick_val = SysTick->VAL;
        password_gen_init_entropy(adc_noise, time_us, systick_val);

        generate_random_password(plaintext_password, GENERATE_PASSWORD_LENGTH);
        size_t pass_len = strlen(plaintext_password);

        // --- 2. ENCRYPT WITH AES-128 ---
        size_t encrypted_bytes = 0;
        aes128_encrypt_padded(
            (const uint8_t *)plaintext_password,
            pass_len,
            MASTER_AES_KEY,
            record.encrypted_data,
            &encrypted_bytes
        );

        record.magic = STORAGE_MAGIC;
        record.password_len = (uint8_t)pass_len;
        record.encrypted_len = (uint8_t)encrypted_bytes;

        // --- 3. STORE ENCRYPTED CREDENTIAL IN FLASH MEMORY ---
        storage_save_record(&record);

        // Clear initial plaintext from memory before proceeding
        secure_zero_memory(plaintext_password, sizeof(plaintext_password));
    }

    // --- 4. RETRIEVE FROM FLASH & DECRYPT CREDENTIAL ---
    if (!storage_load_record(&record)) {
        // If flash read failed or empty, blink error alert
        blink_led(5, 50);
        return;
    }

    size_t decrypted_len = 0;
    int decrypt_ok = aes128_decrypt_padded(
        record.encrypted_data,
        record.encrypted_len,
        MASTER_AES_KEY,
        (uint8_t *)plaintext_password,
        &decrypted_len
    );

    if (!decrypt_ok || decrypted_len == 0) {
        // Decryption failed or corrupted data
        blink_led(6, 60);
        secure_zero_memory(plaintext_password, sizeof(plaintext_password));
        return;
    }

    // --- 5. TRANSMIT VIA USB HID KEYBOARD TO PC ---
    delay(200); // Small grace period before typing

    // Type out each character with slight human-like delay so host OS doesn't drop keystrokes
    for (size_t i = 0; i < decrypted_len; ++i) {
        Keyboard.write((uint8_t)plaintext_password[i]);
        delay(12);
    }

    // Press ENTER to submit form / login
    delay(50);
    Keyboard.write(KEY_RETURN);

    // --- 6. SECURITY: ERASE PLAINTEXT AND CIPHERTEXT IN SRAM ---
    secure_zero_memory(plaintext_password, sizeof(plaintext_password));
    secure_zero_memory(&record, sizeof(record));

    // Confirmation blink
    set_led(false);
    blink_led(2, 100);
}

// ------------------------------------------------------------------------------
// Setup
// ------------------------------------------------------------------------------
void setup() {
    // Configure GPIOs
    pinMode(PIN_BUTTON, INPUT); // Expects external 10k pull-down to GND
    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(PIN_ENTROPY_ADC, INPUT_ANALOG);

    set_led(false); // LED Off initially

    // Initialize Flash/EEPROM storage module
    storage_init();

    // Trigger USB soft disconnect to ensure PC recognizes USB HID enumeration
    trigger_usb_reenumeration();

    // Begin USB HID Keyboard emulation
    Keyboard.begin();

    // Initial startup ready pulse (3 fast blinks)
    blink_led(3, 80);
}

// ------------------------------------------------------------------------------
// Main Event Loop with Debounce & Mode Detection
// ------------------------------------------------------------------------------
void loop() {
    // Check if user pressed button on PA0
    if (digitalRead(PIN_BUTTON) == HIGH) {
        delay(50); // Software debounce delay
        if (digitalRead(PIN_BUTTON) == HIGH) {
            
            // Measure how long the button is held
            uint32_t press_start = millis();
            bool long_press_signaled = false;

            while (digitalRead(PIN_BUTTON) == HIGH) {
                // If held for > 2 seconds, indicate with fast blink
                if (!long_press_signaled && (millis() - press_start > 2000)) {
                    long_press_signaled = true;
                    set_led(true);
                }
                delay(10);
            }
            uint32_t press_duration = millis() - press_start;

            if (press_duration >= 2000) {
                // LONG PRESS (> 2 sec): Re-type the existing saved password from Flash
                execute_credential_pipeline(false);
            } else {
                // SHORT CLICK (< 2 sec): Generate BRAND NEW password, encrypt, store & type!
                execute_credential_pipeline(true);
            }

            // Wait until button is fully released and debounce settled
            delay(300);
        }
    }
    delay(20);
}
