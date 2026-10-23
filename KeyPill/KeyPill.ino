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
 * 2. Rotary Encoder: KY-040 Module on PA1 (CLK), PA2 (DT), PA0 (SW Push Switch)
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
#define PIN_ENCODER_SW   PA0   // KY-040 Push Switch (active LOW with internal pull-up)
#define PIN_ENCODER_CLK  PA1   // KY-040 Rotary Clock
#define PIN_ENCODER_DT   PA2   // KY-040 Rotary Data
#define PIN_ENTROPY_ADC  PA3   // Unconnected floating analog pin for RF/thermal noise

#define PIN_LED_GREEN    PA8   // External Green LED (Success / USB Typing)
#define PIN_LED_RED      PA9   // External Red LED (Encrypting / Busy / Alert)
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

static const char *MENU_LABELS[MENU_TOTAL_ITEMS] = {
    "1. AutoFill New",
    "2. AutoFill Saved",
    "3. View Password",
    "4. Set Length",
    "5. System Info"
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

void trigger_usb_reenumeration(void) {
    pinMode(PA12, OUTPUT);
    digitalWrite(PA12, LOW); // Force D+ low (disconnect)
    delay(150);
    pinMode(PA12, INPUT);   // Release D+ (re-attach)
    delay(150);
}

// ------------------------------------------------------------------------------
// OLED Renderers
// ------------------------------------------------------------------------------
void render_menu_screen() {
    if (!g_oled_available) return;

    OLED.clear();

    // Top Header Banner
    OLED.fillRect(0, 0, 128, 12, 1);
    OLED.setTextColor(0); // Inverted text for banner
    OLED.setCursor(4, 2);
    OLED.print("=== KEYPILL v1.0 ===");

    // Menu list items
    OLED.setTextColor(1);
    for (int8_t i = 0; i < MENU_TOTAL_ITEMS; ++i) {
        int16_t y = 16 + (i * 9);
        if (i == g_menu_selection) {
            // Highlight selected item
            OLED.fillRect(0, y - 1, 128, 9, 1);
            OLED.setTextColor(0);
            OLED.setCursor(2, y);
            OLED.print("> ");
            OLED.print(MENU_LABELS[i]);
            if (i == 3) {
                OLED.print(": ");
                OLED.print((int)g_password_length);
            }
            OLED.setTextColor(1);
        } else {
            OLED.setCursor(8, y);
            OLED.print(MENU_LABELS[i]);
            if (i == 3) {
                OLED.print(": ");
                OLED.print((int)g_password_length);
            }
        }
    }

    OLED.display();
}

void render_action_status(const char *title, const char *step1, const char *step2, const char *step3) {
    if (!g_oled_available) return;
    OLED.clear();
    OLED.fillRect(0, 0, 128, 12, 1);
    OLED.setTextColor(0);
    OLED.setCursor(4, 2);
    OLED.print(title);

    OLED.setTextColor(1);
    if (step1) { OLED.setCursor(4, 18); OLED.print(step1); }
    if (step2) { OLED.setCursor(4, 30); OLED.print(step2); }
    if (step3) { OLED.setCursor(4, 44); OLED.print(step3); }
    OLED.display();
}

// ------------------------------------------------------------------------------
// Execution Pipelines: Generate New vs Auto-Fill Saved
// ------------------------------------------------------------------------------
void execute_auto_fill_new() {
    set_leds(false, true); // Red LED: Busy
    render_action_status("KEYPILL: NEW PASS", "> Harvesting Entropy", "> Randomizing...", NULL);

    char plaintext_pass[MAX_PASSWORD_LEN + 1];
    memset(plaintext_pass, 0, sizeof(plaintext_pass));

    // 1. Physical Entropy Harvesting & Generation
    uint32_t adc_noise = analogRead(PIN_ENTROPY_ADC);
    uint32_t time_us = micros();
    uint32_t systick_val = SysTick->VAL;
    password_gen_init_entropy(adc_noise, time_us, systick_val);

    generate_random_password(plaintext_pass, g_password_length);
    size_t pass_len = strlen(plaintext_pass);

    render_action_status("KEYPILL: NEW PASS", "[✓] Random Generated", "> AES-128 Encrypting", "> Storing Flash...");

    // 2. AES-128 Encryption & Non-Volatile Flash Storage
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
    storage_save_record(&record);

    render_action_status("KEYPILL: NEW PASS", "[✓] AES-128 Encrypted", "[✓] Saved to Flash", ">> Typing to PC... <<");
    set_leds(true, false); // Green LED: Success / Typing

    // 3. USB HID Keystroke Auto-Fill
    delay(200);
    for (size_t i = 0; i < pass_len; ++i) {
        Keyboard.write((uint8_t)plaintext_pass[i]);
        delay(12);
    }
    delay(50);
    Keyboard.write(KEY_RETURN);

    // 4. Memory Scrubbing
    secure_zero_memory(plaintext_pass, sizeof(plaintext_pass));
    secure_zero_memory(&record, sizeof(record));

    render_action_status("KEYPILL: SUCCESS", "[✓] Pass Generated!", "[✓] Typed to Host PC", "[✓] SRAM Scrubbed!");
    delay(1500);
    set_leds(false, false);
}

void execute_auto_fill_saved() {
    set_leds(false, true); // Red LED: Busy
    render_action_status("KEYPILL: SAVED PASS", "> Reading Flash...", "> Decrypting AES...", NULL);

    struct PasswordRecord record;
    memset(&record, 0, sizeof(record));

    if (!storage_load_record(&record)) {
        render_action_status("KEYPILL: ERROR", "[!] No Password Found", "Flash is empty!", "Click to return...");
        blink_led(PIN_LED_RED, 4, 100);
        set_leds(false, false);
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

    if (!ok || dec_len == 0) {
        render_action_status("KEYPILL: ERROR", "[!] Decryption Failed", "Corrupt payload!", "Click to return...");
        blink_led(PIN_LED_RED, 4, 100);
        set_leds(false, false);
        delay(1500);
        return;
    }

    render_action_status("KEYPILL: SAVED PASS", "[✓] Read Flash OK", "[✓] Decrypted AES-128", ">> Typing to PC... <<");
    set_leds(true, false); // Green LED: Typing

    delay(200);
    for (size_t i = 0; i < dec_len; ++i) {
        Keyboard.write((uint8_t)plaintext_pass[i]);
        delay(12);
    }
    delay(50);
    Keyboard.write(KEY_RETURN);

    secure_zero_memory(plaintext_pass, sizeof(plaintext_pass));
    secure_zero_memory(&record, sizeof(record));

    render_action_status("KEYPILL: SUCCESS", "[✓] Password Auto-Filled", "[✓] ENTER Submitted", "Returning to menu...");
    delay(1200);
    set_leds(false, false);
}

void execute_view_password() {
    struct PasswordRecord record;
    memset(&record, 0, sizeof(record));

    if (!storage_load_record(&record)) {
        render_action_status("KEYPILL: EMPTY", "[!] No Saved Password", "Generate one first!", "Click to return...");
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
        render_action_status("KEYPILL: ERROR", "[!] Decrypt Error", "Corrupt data!", "Click to return...");
        delay(1500);
        return;
    }

    // Display password on screen with a live 6-second countdown bar
    uint32_t start_time = millis();
    const uint32_t view_duration = 6000; // 6 seconds

    while (millis() - start_time < view_duration) {
        uint32_t elapsed = millis() - start_time;
        uint8_t percent = 100 - (uint8_t)((elapsed * 100) / view_duration);

        OLED.clear();
        OLED.fillRect(0, 0, 128, 12, 1);
        OLED.setTextColor(0);
        OLED.setCursor(4, 2);
        OLED.print("STORED CREDENTIAL");

        OLED.setTextColor(1);
        OLED.setCursor(4, 18);
        OLED.print("Pass: ");
        OLED.print(plaintext_pass);

        OLED.setCursor(4, 34);
        OLED.print("Auto-clearing in:");
        OLED.print((int)((view_duration - elapsed) / 1000 + 1));
        OLED.print("s");

        OLED.drawProgressBar(4, 48, 120, 8, percent);
        OLED.display();

        encoder.update();
        if (encoder.isClicked()) {
            break; // Allow user to exit early by clicking knob
        }
        delay(50);
    }

    // Security scrubbing
    secure_zero_memory(plaintext_pass, sizeof(plaintext_pass));
    secure_zero_memory(&record, sizeof(record));
    OLED.clear();
    OLED.display();
}

void execute_set_length() {
    bool selecting = true;
    while (selecting) {
        encoder.update();
        int8_t rot = encoder.getRotation();
        if (rot > 0) {
            if (g_password_length == 12) g_password_length = 14;
            else if (g_password_length == 14) g_password_length = 16;
        } else if (rot < 0) {
            if (g_password_length == 16) g_password_length = 14;
            else if (g_password_length == 14) g_password_length = 12;
        }

        if (encoder.isClicked()) {
            selecting = false; // Confirmed
        }

        OLED.clear();
        OLED.fillRect(0, 0, 128, 12, 1);
        OLED.setTextColor(0);
        OLED.setCursor(4, 2);
        OLED.print("CONFIG PASS LENGTH");

        OLED.setTextColor(1);
        OLED.setCursor(16, 22);
        OLED.setTextSize(2);
        OLED.print("< ");
        OLED.print((int)g_password_length);
        OLED.print(" >");
        OLED.setTextSize(1);

        OLED.setCursor(4, 44);
        OLED.print("Rotate: 12/14/16 chars");
        OLED.setCursor(4, 54);
        OLED.print("Click knob to save");
        OLED.display();

        delay(20);
    }
}

void execute_sys_info() {
    bool viewing = true;
    while (viewing) {
        encoder.update();
        if (encoder.isClicked()) {
            viewing = false;
        }

        OLED.clear();
        OLED.fillRect(0, 0, 128, 12, 1);
        OLED.setTextColor(0);
        OLED.setCursor(4, 2);
        OLED.print("SYSTEM INFORMATION");

        OLED.setTextColor(1);
        OLED.setCursor(4, 16);
        OLED.print("MCU: STM32F103 @72MHz");
        OLED.setCursor(4, 27);
        OLED.print("Crypto: AES-128 NIST");
        OLED.setCursor(4, 38);
        OLED.print("Flash: Page 63 (64KB)");
        OLED.setCursor(4, 49);
        OLED.print("USB: HID Keyboard");
        OLED.setCursor(4, 57);
        OLED.print("Click knob to exit");
        OLED.display();

        delay(30);
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

    set_leds(false, false);

    // Storage and Rotary Encoder initialization
    storage_init();
    encoder.begin();

    // OLED Display initialization over I2C (PB6 = SCL, PB7 = SDA)
    g_oled_available = OLED.begin(OLED_I2C_ADDR);

    // USB Re-enumeration on PA12 to ensure host PC detects keyboard
    trigger_usb_reenumeration();
    Keyboard.begin();

    // Welcome Splash Screen on OLED
    if (g_oled_available) {
        OLED.clear();
        OLED.drawRect(0, 0, 128, 64, 1);
        OLED.setCursor(14, 14);
        OLED.setTextSize(2);
        OLED.print("KEYPILL");
        OLED.setTextSize(1);
        OLED.setCursor(10, 34);
        OLED.print("Hardware Password Token");
        OLED.setCursor(24, 48);
        OLED.print("STM32 + AES-128");
        OLED.display();
        delay(1200);
    }

    render_menu_screen();
}

void loop() {
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

    delay(10);
}
