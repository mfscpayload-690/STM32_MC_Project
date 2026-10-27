/**
 * ==============================================================================
 * Project: KeyPill - Secure Hardware Password Generator & Auto-Filler
 * Target Hardware: STM32F103C8T6 (STM32 Blue Pill) + ST-Link V2
 * Environment: Arduino IDE with STM32duino (Official STMicroelectronics Core)
 * Author: Group 2 (Aravind Lal, Betsa Mariya Philip, Karthik V Panicker, Vaishnav Prasad A)
 * Course: Microcontrollers (PBCST504) - KTU
 * ==============================================================================
 *
 * HARDWARE PERIPHERALS:
 * 1. OLED Display: 0.96" 128x64 I2C (SSD1306) on PB6 (SCL) and PB7 (SDA)
 * 2. Rotary Encoder: EC11 Module on PA1 (S1/CLK), PA2 (S2/DT), PA0 (Key/SW Push Switch)
 * 3. Status LEDs:
 *    - Green LED on PA8 (Operation Success / Ready / Typing)
 *    - Red LED on PA9 (Busy / Cryptographic Operation / Error)
 *    - Onboard LED on PC13 (Heartbeat / Activity)
 * 4. USB Interface: USB 2.0 Full Speed HID Keyboard on PA11 (D-) and PA12 (D+)
 * 5. Floating ADC: PA3 (Physical entropy harvesting)
 *
 * MENU NAVIGATION:
 * - Rotate Knob: Scroll through options or adjust password length
 * - Click Knob (Push SW): Select option / Confirm
 *
 * MENU OPTIONS:
 * 1. Auto-Fill New   -> Generate new password, AES-128 encrypt, store in Flash & type via USB
 * 2. Auto-Fill Saved -> Load from Flash, AES-128 decrypt & type via USB
 * 3. View Password   -> Display decrypted password on OLED with 6-second auto-clear countdown
 * 4. Set Length      -> Configure password length (12, 14, 16 characters)
 * 5. System Info     -> Display device specs, crypto engine details & Flash status
 * ==============================================================================
 */

#ifndef GCC_VERSION
#define GCC_VERSION 140201
#endif
#include <Arduino.h>
#include <Keyboard.h>
#include "aes128.h"
#include "password_gen.h"
#include "storage.h"
#include "oled_ssd1306.h"
#include "rotary_encoder.h"

// ------------------------------------------------------------------------------
// Hardware Pin Assignments
// ------------------------------------------------------------------------------
#define PIN_ENCODER_SW   PA0   // EC11 Key / Push Switch (active LOW with internal pull-up)
#define PIN_ENCODER_CLK  PA1   // EC11 S1 (Quadrature Clock / Phase A)
#define PIN_ENCODER_DT   PA2   // EC11 S2 (Quadrature Data / Phase B)
#define PIN_ENTROPY_ADC  PA3   // Unconnected floating analog pin for RF/thermal noise

#define PIN_LED_GREEN    PA9   // External Green LED (Success / Standby / USB Typing)
#define PIN_LED_RED      PA8   // External Red LED (Encrypting / Busy / Alert)
#define PIN_STATUS_LED   PC13  // Onboard Blue Pill LED (active LOW: LOW = ON)

// ------------------------------------------------------------------------------
// Security & System Settings
// ------------------------------------------------------------------------------
// 128-bit AES Master Key (16 bytes)
static const uint8_t MASTER_AES_KEY[16] = {
    0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};

static uint8_t g_password_length = DEFAULT_PASSWORD_LEN; // Default: 14 chars
static bool g_oled_available = false;
static bool g_usb_ready = false;

void ensure_usb_ready(void) {
    if (!g_usb_ready) {
        Keyboard.begin();
        g_usb_ready = true;
        delay(350);
    }
}

// Instantiate Rotary Encoder driver
RotaryEncoder encoder(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW);

// ------------------------------------------------------------------------------
// Menu System State Machine
// ------------------------------------------------------------------------------
enum AppState {
    STATE_MENU,
    STATE_EXEC_NEW,
    STATE_EXEC_SAVED,
    STATE_VIEW_PASS,
    STATE_SET_LENGTH,
    STATE_SYS_INFO
};

static AppState g_current_state = STATE_MENU;
static int8_t   g_menu_selection = 0;
#define MENU_TOTAL_ITEMS 5

struct CardInfo {
    const char *num_str;  // "1/5", "2/5", etc.
    const char *title;    // "1. AUTO-FILL NEW"
    const char *desc;     // "Generate & Type to PC"
    const char *badge;    // "AES-128 Flash"
};

static const CardInfo CARDS[MENU_TOTAL_ITEMS] = {
    { "1/5", "1. AUTO-FILL NEW",   "Next Free Slot (1-10)", "AES-128 Flash" },
    { "2/5", "2. AUTO-FILL SAVED", "Choose Slot & Type",    "10-Slot Vault" },
    { "3/5", "3. VIEW PASSWORD",   "Inspect Stored Slot",   "6s Auto-Clear" },
    { "4/5", "4. SET LENGTH",      "Configure Password",    "12 / 14 / 16 chars" },
    { "5/5", "5. SYSTEM INFO",     "Device Diagnostics",    "STM32 Cortex-M3" }
};

// ------------------------------------------------------------------------------
// LED & Memory Utilities
// ------------------------------------------------------------------------------
void set_leds(bool green, bool red) {
    digitalWrite(PIN_LED_GREEN, green ? HIGH : LOW);
    digitalWrite(PIN_LED_RED,   red   ? HIGH : LOW);
    digitalWrite(PIN_STATUS_LED, (green || red) ? LOW : HIGH); // PC13 active LOW
}

void blink_led(uint8_t pin, int times, int delay_ms) {
    for (int i = 0; i < times; ++i) {
        digitalWrite(pin, HIGH);
        delay(delay_ms);
        digitalWrite(pin, LOW);
        delay(delay_ms);
    }
}

void secure_zero_memory(void *v, size_t n) {
    volatile uint8_t *p = (volatile uint8_t *)v;
    while (n--) {
        *p++ = 0;
    }
}

// ------------------------------------------------------------------------------
// OLED Card & Carousel Renderers (Single Option at a Time with Borders)
// ------------------------------------------------------------------------------
void render_menu_screen() {
    set_leds(true, false); // Green LED ON = System Ready in Menu
    if (!g_oled_available) return;

    OLED.clear();

    // 1. Sleek Outer Border for Entire Screen
    OLED.drawRoundRect(0, 0, 128, 64, 4, 1);

    // 2. Top Header Bar
    OLED.setTextSize(1);
    OLED.setTextColor(1);
    OLED.setCursor(6, 4);
    OLED.print("KEYPILL");

    // Top Right Option Counter (e.g. "1/5", "2/5")
    OLED.setCursor(98, 4);
    OLED.print(CARDS[g_menu_selection].num_str);

    // Divider Line below Header
    OLED.drawLine(4, 13, 123, 13, 1);

    // 3. Central Single-Option Card with Nice Border
    OLED.drawRoundRect(6, 16, 116, 33, 3, 1);

    // Inverted Title Banner inside Card
    OLED.fillRoundRect(8, 18, 112, 12, 2, 1);
    OLED.setTextColor(0); // Black text on white banner
    OLED.setCursor(12, 20);
    OLED.print(CARDS[g_menu_selection].title);

    // Subtitle Description inside Card
    OLED.setTextColor(1); // Normal white text
    OLED.setCursor(10, 34);
    if (g_menu_selection == 0) {
        int empty_s = storage_find_first_empty_slot();
        if (empty_s >= 0) {
            OLED.print("Next: Slot ");
            OLED.print(empty_s + 1);
            OLED.print("/10");
        } else {
            OLED.print("Vault Full (10/10)");
        }
    } else if (g_menu_selection == 1 || g_menu_selection == 2) {
        OLED.print("Vault: ");
        OLED.print((int)storage_count_occupied());
        OLED.print("/10 slots used");
    } else if (g_menu_selection == 3) {
        OLED.print("Length: ");
        OLED.print((int)g_password_length);
        OLED.print(" characters");
    } else {
        OLED.print(CARDS[g_menu_selection].desc);
    }

    // 4. Bottom Footer Navigation Hints
    OLED.drawLine(4, 51, 123, 51, 1);
    OLED.setCursor(10, 53);
    OLED.print("< Turn >   [SELECT]");

    OLED.display();
}

void render_action_card(const char *title, const char *step_text, uint8_t progress_pct) {
    if (!g_oled_available) return;
    OLED.clear();
    OLED.setTextSize(1);
    OLED.setTextColor(1);
    OLED.setCursor(4, 2);
    OLED.print(title);
    OLED.drawLine(0, 11, 127, 11, 1);

    OLED.drawRoundRect(2, 14, 124, 48, 3, 1);
    OLED.setCursor(8, 20);
    OLED.print(step_text);

    OLED.drawProgressBar(8, 34, 112, 10, progress_pct);

    OLED.setCursor(52, 48);
    OLED.print((int)progress_pct);
    OLED.print("%");

    OLED.display();
}

void render_success_card(const char *title, const char *line1, const char *line2) {
    if (!g_oled_available) return;
    OLED.clear();
    OLED.setTextSize(1);
    OLED.setTextColor(1);
    OLED.setCursor(4, 2);
    OLED.print("KEYPILL: SUCCESS");
    OLED.drawLine(0, 11, 127, 11, 1);

    OLED.drawRoundRect(2, 14, 124, 48, 3, 1);
    OLED.fillRoundRect(6, 17, 116, 12, 2, 1);
    OLED.setTextColor(0);
    OLED.setCursor(8, 19);
    OLED.print(title);

    OLED.setTextColor(1);
    if (line1) { OLED.setCursor(8, 33); OLED.print(line1); }
    if (line2) { OLED.setCursor(8, 45); OLED.print(line2); }

    OLED.display();
}

void render_error_card(const char *title, const char *line1, const char *line2) {
    if (!g_oled_available) return;
    OLED.clear();
    OLED.setTextSize(1);
    OLED.setTextColor(1);
    OLED.setCursor(4, 2);
    OLED.print("KEYPILL: ALERT");
    OLED.drawLine(0, 11, 127, 11, 1);

    OLED.drawRoundRect(2, 14, 124, 48, 3, 1);
    OLED.fillRoundRect(6, 17, 116, 12, 2, 1);
    OLED.setTextColor(0);
    OLED.setCursor(8, 19);
    OLED.print(title);

    OLED.setTextColor(1);
    if (line1) { OLED.setCursor(8, 33); OLED.print(line1); }
    if (line2) { OLED.setCursor(8, 45); OLED.print(line2); }

    OLED.display();
}

// ------------------------------------------------------------------------------
// Multi-Slot Browser UI (Vertical Overview List of 10 Slots)
// ------------------------------------------------------------------------------
int select_slot_browser(const char *action_title, bool allow_clear) {
    int8_t sel_slot = 0;
    bool browsing = true;
    bool needs_redraw = true;

    while (browsing) {
        encoder.update();
        int8_t rot = encoder.getRotation();
        if (rot != 0) {
            sel_slot += rot;
            if (sel_slot < 0) sel_slot = STORAGE_NUM_SLOTS - 1;
            if (sel_slot >= STORAGE_NUM_SLOTS) sel_slot = 0;
            needs_redraw = true;
        }

        // Long-press detection to clear slot (held >= 1500ms)
        if (allow_clear && encoder.isHeld(1500)) {
            if (storage_is_slot_occupied((uint8_t)sel_slot)) {
                // Confirm clear prompt
                OLED.clear();
                OLED.drawRoundRect(0, 0, 128, 64, 4, 1);
                OLED.fillRoundRect(6, 6, 116, 14, 2, 1);
                OLED.setTextColor(0);
                OLED.setCursor(14, 9);
                OLED.print("CONFIRM CLEAR?");
                OLED.setTextColor(1);
                OLED.setCursor(12, 26);
                OLED.print("Delete Slot ");
                if (sel_slot + 1 < 10) OLED.print("0");
                OLED.print(sel_slot + 1);
                OLED.print("?");
                OLED.setCursor(8, 39);
                OLED.print("Click: YES (Delete)");
                OLED.setCursor(8, 49);
                OLED.print("Turn:  CANCEL");
                OLED.display();

                // Wait for release
                delay(300);
                while (encoder.isPressed()) { encoder.update(); delay(5); }
                delay(100);

                bool awaiting = true;
                while (awaiting) {
                    encoder.update();
                    if (encoder.isClicked()) {
                        storage_clear_slot((uint8_t)sel_slot);
                        blink_led(PIN_LED_GREEN, 3, 80);
                        render_success_card("SLOT CLEARED!", "Flash slot erased", "Slot is now free");
                        delay(1000);
                        awaiting = false;
                        needs_redraw = true;
                        break;
                    }
                    if (encoder.getRotation() != 0) {
                        awaiting = false;
                        needs_redraw = true;
                        break;
                    }
                    delay(5);
                }
            }
        }

        if (encoder.isClicked()) {
            return (int)sel_slot;
        }

        if (needs_redraw) {
            needs_redraw = false;

            OLED.clear();
            OLED.drawRoundRect(0, 0, 128, 64, 4, 1);

            // Header
            OLED.setTextSize(1);
            OLED.setTextColor(1);
            OLED.setCursor(6, 4);
            OLED.print(action_title);
            OLED.setCursor(96, 4);
            OLED.print((int)storage_count_occupied());
            OLED.print("/10");
            OLED.drawLine(4, 13, 123, 13, 1);

            // Display 4 visible slots in scrolling window
            int8_t start_slot = sel_slot - 1;
            if (start_slot < 0) start_slot = 0;
            if (start_slot > 6) start_slot = 6;

            for (uint8_t i = 0; i < 4; ++i) {
                uint8_t s = start_slot + i;
                int16_t y = 16 + (i * 9);
                bool is_sel = (s == sel_slot);
                bool occ = storage_is_slot_occupied(s);

                struct PasswordRecord rec;
                uint8_t plen = 0;
                if (occ && storage_load_slot(s, &rec)) {
                    plen = rec.password_len;
                }

                if (is_sel) {
                    OLED.fillRoundRect(6, y - 1, 116, 9, 2, 1);
                    OLED.setTextColor(0); // Inverted text
                } else {
                    OLED.setTextColor(1);
                }

                // Format: [01] 14c  ACTIVE   or   [01] ---  EMPTY
                OLED.setCursor(8, y);
                if (s + 1 < 10) OLED.print("[0");
                else OLED.print("[");
                OLED.print(s + 1);
                OLED.print("] ");

                if (occ) {
                    OLED.print(plen);
                    OLED.print("c  ACTIVE");
                } else {
                    OLED.print("---  EMPTY");
                }
            }

            // Footer
            OLED.setTextColor(1);
            OLED.drawLine(4, 51, 123, 51, 1);
            OLED.setCursor(6, 53);
            OLED.print("Click:OK | Hold:DEL");

            OLED.display();
        }

        delay(1);
    }

    return -1;
}

// ------------------------------------------------------------------------------
// Execution Pipelines: Generate New vs Auto-Fill Saved
// ------------------------------------------------------------------------------
void execute_auto_fill_new() {
    // 1. Sequential search for next empty slot (0 to 9)
    int slot = storage_find_first_empty_slot();

    if (slot == -1) {
        // Vault is 10/10 FULL!
        blink_led(PIN_LED_RED, 4, 100);
        render_error_card("VAULT FULL (10/10)", "All 10 slots filled!", "Choose slot to replace");
        delay(1600);

        slot = select_slot_browser("OVERWRITE SLOT", false);
        if (slot < 0) return; // Cancelled
    }

    char slot_label[32];
    snprintf(slot_label, sizeof(slot_label), "Saved to Slot %02d", slot + 1);

    set_leds(false, true); // Red LED: Busy
    render_action_card("AUTO-FILL NEW", "1/3 Harvesting Entropy...", 25);
    delay(200);

    char plaintext_pass[MAX_PASSWORD_LEN + 1];
    memset(plaintext_pass, 0, sizeof(plaintext_pass));

    // Entropy & Random generation
    uint32_t adc_noise = analogRead(PIN_ENTROPY_ADC);
    uint32_t time_us = micros();
    uint32_t systick_val = SysTick->VAL;
    password_gen_init_entropy(adc_noise, time_us, systick_val);

    generate_random_password(plaintext_pass, g_password_length);
    size_t pass_len = strlen(plaintext_pass);

    // AES-128 Encryption & Save to specific Slot
    render_action_card("AUTO-FILL NEW", "2/3 AES-128 Encrypting...", 60);
    delay(200);

    struct PasswordRecord record;
    memset(&record, 0, sizeof(record));
    size_t encrypted_bytes = 0;

    aes128_encrypt_padded(
        (const uint8_t *)plaintext_pass,
        pass_len,
        MASTER_AES_KEY,
        record.encrypted_data,
        &encrypted_bytes
    );

    record.magic = STORAGE_MAGIC;
    record.password_len = (uint8_t)pass_len;
    record.encrypted_len = (uint8_t)encrypted_bytes;
    storage_save_slot((uint8_t)slot, &record);

    // USB HID typing with pulsing Green LED
    ensure_usb_ready();
    delay(100);

    for (size_t i = 0; i < pass_len; ++i) {
        digitalWrite(PIN_LED_GREEN, (i % 2 == 0) ? HIGH : LOW);
        digitalWrite(PIN_LED_RED, LOW);
        Keyboard.write((uint8_t)plaintext_pass[i]);
        uint8_t pct = 60 + (uint8_t)(((i + 1) * 40) / pass_len);
        render_action_card("AUTO-FILL NEW", "3/3 Typing via USB...", pct);
        delay(35);
    }
    Keyboard.write(KEY_RETURN);

    secure_zero_memory(plaintext_pass, sizeof(plaintext_pass));
    secure_zero_memory(&record, sizeof(record));

    set_leds(true, false); // Green LED Solid: Success
    render_success_card("PASSWORD GENERATED!", slot_label, "Typed & Saved to Flash");
    delay(1500);
}

void execute_auto_fill_saved() {
    if (storage_count_occupied() == 0) {
        render_error_card("NO SAVED PASSWORDS", "Vault is empty!", "Generate one first.");
        blink_led(PIN_LED_RED, 4, 100);
        delay(1500);
        return;
    }

    int slot = select_slot_browser("AUTO-FILL USB", true);
    if (slot < 0) return;

    if (!storage_is_slot_occupied((uint8_t)slot)) {
        render_error_card("EMPTY SLOT", "No password stored here!", "Select an active slot");
        blink_led(PIN_LED_RED, 2, 100);
        delay(1400);
        return;
    }

    set_leds(false, true); // Red LED: Busy
    render_action_card("AUTO-FILL SAVED", "Reading Slot Data...", 30);
    delay(200);

    struct PasswordRecord record;
    memset(&record, 0, sizeof(record));

    if (!storage_load_slot((uint8_t)slot, &record)) {
        render_error_card("READ ERROR", "Corrupt Flash Payload!", "Please regenerate.");
        blink_led(PIN_LED_RED, 4, 100);
        delay(1500);
        return;
    }

    render_action_card("AUTO-FILL SAVED", "Decrypting AES-128...", 60);
    delay(200);

    char plaintext_pass[MAX_PASSWORD_LEN + 1];
    memset(plaintext_pass, 0, sizeof(plaintext_pass));
    size_t dec_len = 0;

    int ok = aes128_decrypt_padded(
        record.encrypted_data,
        record.encrypted_len,
        MASTER_AES_KEY,
        (uint8_t *)plaintext_pass,
        &dec_len
    );

    if (!ok || dec_len == 0) {
        render_error_card("DECRYPT FAILED", "Corrupt Flash Payload!", "Please regenerate.");
        blink_led(PIN_LED_RED, 4, 100);
        delay(1500);
        return;
    }

    ensure_usb_ready();
    delay(100);

    for (size_t i = 0; i < dec_len; ++i) {
        digitalWrite(PIN_LED_GREEN, (i % 2 == 0) ? HIGH : LOW);
        digitalWrite(PIN_LED_RED, LOW);
        Keyboard.write((uint8_t)plaintext_pass[i]);
        uint8_t pct = 60 + (uint8_t)(((i + 1) * 40) / dec_len);
        render_action_card("AUTO-FILL SAVED", "Typing via USB...", pct);
        delay(35);
    }
    Keyboard.write(KEY_RETURN);

    secure_zero_memory(plaintext_pass, sizeof(plaintext_pass));
    secure_zero_memory(&record, sizeof(record));

    char slot_label[32];
    snprintf(slot_label, sizeof(slot_label), "Slot %02d Auto-Filled!", slot + 1);

    set_leds(true, false); // Green LED Solid: Success
    render_success_card("AUTO-FILL COMPLETE!", slot_label, "Submitted to PC");
    delay(1500);
}

void execute_view_password() {
    if (storage_count_occupied() == 0) {
        render_error_card("NO SAVED PASSWORDS", "Vault is empty!", "Generate one first.");
        blink_led(PIN_LED_RED, 4, 100);
        delay(1500);
        return;
    }

    int slot = select_slot_browser("VIEW PASSWORD", true);
    if (slot < 0) return;

    if (!storage_is_slot_occupied((uint8_t)slot)) {
        render_error_card("EMPTY SLOT", "No password stored here!", "Select an active slot");
        blink_led(PIN_LED_RED, 2, 100);
        delay(1400);
        return;
    }

    set_leds(false, true); // Red LED: Busy
    struct PasswordRecord record;
    memset(&record, 0, sizeof(record));

    if (!storage_load_slot((uint8_t)slot, &record)) {
        render_error_card("READ ERROR", "Corrupt Flash Payload!", "Please regenerate.");
        blink_led(PIN_LED_RED, 4, 100);
        delay(1500);
        return;
    }

    char plaintext_pass[MAX_PASSWORD_LEN + 1];
    memset(plaintext_pass, 0, sizeof(plaintext_pass));
    size_t dec_len = 0;

    int ok = aes128_decrypt_padded(
        record.encrypted_data,
        record.encrypted_len,
        MASTER_AES_KEY,
        (uint8_t *)plaintext_pass,
        &dec_len
    );

    if (!ok) {
        render_error_card("DECRYPT FAILED", "Corrupt Flash Payload!", "Please regenerate.");
        blink_led(PIN_LED_RED, 4, 100);
        delay(1500);
        return;
    }

    set_leds(true, false); // Green LED: Ready viewing

    char header_str[32];
    snprintf(header_str, sizeof(header_str), "SLOT %02d PASS", slot + 1);

    // Display password with live 6-second countdown bar
    uint32_t start_time = millis();
    const uint32_t view_duration = 6000; // 6 seconds

    while (millis() - start_time < view_duration) {
        uint32_t elapsed = millis() - start_time;
        uint32_t remaining_ms = (view_duration > elapsed) ? (view_duration - elapsed) : 0;
        uint8_t percent = (uint8_t)((remaining_ms * 100) / view_duration);

        OLED.clear();

        // Header with remaining seconds
        OLED.setTextSize(1);
        OLED.setTextColor(1);
        OLED.setCursor(4, 2);
        OLED.print(header_str);
        OLED.setCursor(102, 2);
        OLED.print((int)(remaining_ms / 1000) + 1);
        OLED.print("s");
        OLED.drawLine(0, 11, 127, 11, 1);

        // Card Box
        OLED.drawRoundRect(2, 14, 124, 34, 3, 1);
        OLED.setCursor(8, 18);
        OLED.print("Length: ");
        OLED.print((int)dec_len);
        OLED.print(" chars");

        // Display Password in highlighted box
        OLED.fillRect(6, 28, 116, 16, 1);
        OLED.setTextColor(0); // Inverted text
        OLED.setCursor(10, 32);
        OLED.print(plaintext_pass);

        // Countdown progress bar at bottom
        OLED.setTextColor(1);
        OLED.drawProgressBar(4, 52, 120, 8, percent);

        OLED.display();

        // Check if user clicked to exit early
        encoder.update();
        if (encoder.isClicked()) {
            break;
        }
        delay(40);
    }

    // Security scrubbing
    secure_zero_memory(plaintext_pass, sizeof(plaintext_pass));
    secure_zero_memory(&record, sizeof(record));
    OLED.clear();
    OLED.display();
}

void execute_set_length() {
    static const uint8_t LENGTHS[3] = { 12, 14, 16 };
    int8_t idx = 1; // Default 14
    if (g_password_length == 12) idx = 0;
    else if (g_password_length == 16) idx = 2;

    bool needs_redraw = true;
    bool selecting = true;

    while (selecting) {
        encoder.update();
        int8_t rot = encoder.getRotation();
        if (rot != 0) {
            idx += rot;
            if (idx < 0) idx = 2;
            if (idx > 2) idx = 0;
            needs_redraw = true; // Only redraw when knob moves!
        }

        if (encoder.isClicked()) {
            g_password_length = LENGTHS[idx];
            selecting = false;
            break;
        }

        if (needs_redraw) {
            needs_redraw = false;

            OLED.clear();
            OLED.drawRoundRect(0, 0, 128, 64, 4, 1);

            // Header
            OLED.setTextSize(1);
            OLED.setTextColor(1);
            OLED.setCursor(6, 4);
            OLED.print("KEYPILL");
            OLED.setCursor(76, 4);
            OLED.print("SET LENGTH");
            OLED.drawLine(4, 13, 123, 13, 1);

            // Card container
            OLED.drawRoundRect(6, 16, 116, 33, 3, 1);
            OLED.setCursor(10, 18);
            OLED.print("Select Key Length:");

            // Draw 3 length options with radio-style rounded indicator
            for (int i = 0; i < 3; ++i) {
                int16_t opt_x = 10 + (i * 38);
                if (i == idx) {
                    OLED.fillRoundRect(opt_x, 28, 34, 16, 2, 1);
                    OLED.setTextColor(0);
                    OLED.setCursor(opt_x + 6, 32);
                    OLED.print((int)LENGTHS[i]);
                    OLED.print("c");
                    OLED.setTextColor(1);
                } else {
                    OLED.drawRoundRect(opt_x, 28, 34, 16, 2, 1);
                    OLED.setCursor(opt_x + 6, 32);
                    OLED.print((int)LENGTHS[i]);
                    OLED.print("c");
                }
            }

            // Footer
            OLED.drawLine(4, 51, 123, 51, 1);
            OLED.setCursor(10, 53);
            OLED.print("< Turn >   [CONFIRM]");
            OLED.display();
        }

        delay(1); // 1,000Hz sampling for instantaneous knob response
    }

    render_success_card("LENGTH UPDATED!", "New key length configured", "Ready for auto-fill");
    delay(1000);
}

void execute_sys_info() {
    OLED.clear();
    OLED.drawRoundRect(0, 0, 128, 64, 4, 1);

    // Header
    OLED.setTextSize(1);
    OLED.setTextColor(1);
    OLED.setCursor(6, 4);
    OLED.print("KEYPILL");
    OLED.setCursor(76, 4);
    OLED.print("SYS INFO");
    OLED.drawLine(4, 13, 123, 13, 1);

    // Details box
    OLED.drawRoundRect(6, 16, 116, 33, 3, 1);
    OLED.setCursor(10, 19);
    OLED.print("MCU: STM32F103 @72MHz");
    OLED.setCursor(10, 28);
    OLED.print("Core: ARM Cortex-M3");
    OLED.setCursor(10, 37);
    OLED.print("Crypto: AES-128 NIST");

    // Footer
    OLED.drawLine(4, 51, 123, 51, 1);
    OLED.setCursor(10, 53);
    OLED.print("[ Click to return ]");
    OLED.display();

    bool viewing = true;
    while (viewing) {
        encoder.update();
        if (encoder.isClicked() || encoder.getRotation() != 0) {
            viewing = false;
            break;
        }
        delay(5);
    }
}

// ------------------------------------------------------------------------------
// Arduino Setup & Main Loop
// ------------------------------------------------------------------------------
void setup() {
    // GPIO initialization
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_STATUS_LED, OUTPUT);
    pinMode(PIN_ENTROPY_ADC, INPUT_ANALOG);

    // Hardware Power-on Self-Test: flash all LEDs for 500ms so user verifies LEDs and power
    digitalWrite(PIN_LED_GREEN, HIGH);
    digitalWrite(PIN_LED_RED, HIGH);
    digitalWrite(PIN_STATUS_LED, LOW); // Onboard PC13 active LOW (ON)
    delay(500);
    set_leds(false, false);
    delay(150);
    set_leds(true, false); // Green LED ON = Ready!

    // Storage and Rotary Encoder initialization
    storage_init();
    encoder.begin();

    // OLED Display initialization over I2C (PB6 = SCL, PB7 = SDA)
    g_oled_available = OLED.begin(OLED_I2C_ADDR);
    if (!g_oled_available) {
        // Fallback for OLED modules wired at 0x3D
        g_oled_available = OLED.begin(0x3D);
    }

    // Welcome Splash Screen on OLED with KeyPill Branding & Double Border (4s duration)
    if (g_oled_available) {
        for (uint8_t p = 0; p <= 100; p += 5) {
            OLED.clear();

            // Double border (outer rounded + inner rect)
            OLED.drawRoundRect(0, 0, 128, 64, 4, 1);
            OLED.drawRect(2, 2, 124, 60, 1);

            // Inverted Brand Header Banner
            OLED.fillRoundRect(14, 6, 100, 19, 3, 1);
            OLED.setTextSize(2);
            OLED.setTextColor(0); // Inverted text
            OLED.setCursor(24, 8);
            OLED.print("KEYPILL");

            // Subtitle Brand Details
            OLED.setTextSize(1);
            OLED.setTextColor(1);
            OLED.drawLine(8, 28, 119, 28, 1);
            OLED.setCursor(10, 32);
            OLED.print("HARDWARE KEY VAULT");
            OLED.setCursor(12, 42);
            OLED.print("AES-128 NIST CRYPTO");

            // Animated Boot Progress Bar
            OLED.drawProgressBar(14, 52, 100, 6, p);
            OLED.display();
            delay(90); // 21 steps * 90ms = ~1.9 seconds smooth fill
        }
        delay(2100);   // Hold completed screen for 2.1s -> Total boot time = 4.0 seconds
    }

    render_menu_screen();
}

void loop() {
    // Active heartbeat: toggle onboard PC13 LED every 500ms so user knows loop is running
    static uint32_t last_heartbeat = 0;
    if (millis() - last_heartbeat >= 500) {
        last_heartbeat = millis();
        digitalWrite(PIN_STATUS_LED, !digitalRead(PIN_STATUS_LED));
    }

    // Read Rotary Encoder knob movements
    encoder.update();

    int8_t rot = encoder.getRotation();
    if (rot != 0) {
        g_menu_selection += rot;
        if (g_menu_selection < 0) g_menu_selection = MENU_TOTAL_ITEMS - 1;
        if (g_menu_selection >= MENU_TOTAL_ITEMS) g_menu_selection = 0;
        render_menu_screen();
    }

    // Check if user clicked the rotary knob (Push SW on PA0)
    if (encoder.isClicked()) {
        switch (g_menu_selection) {
            case 0:
                execute_auto_fill_new();
                break;
            case 1:
                execute_auto_fill_saved();
                break;
            case 2:
                execute_view_password();
                break;
            case 3:
                execute_set_length();
                break;
            case 4:
                execute_sys_info();
                break;
        }
        render_menu_screen();
    }

    delay(2); // Ultra-responsive 500Hz polling rate for seamless rotary scrolling
}
