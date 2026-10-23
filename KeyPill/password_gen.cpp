/**
 * @file password_gen.cpp
 * @brief High-entropy password generator implementation using Xorshift128+
 */

#include "password_gen.h"
#include <string.h>

// Character pools
static const char UPPERCASE_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char LOWERCASE_CHARS[] = "abcdefghijklmnopqrstuvwxyz";
static const char DIGIT_CHARS[]     = "0123456789";
static const char SYMBOL_CHARS[]    = "!@#$%^&*()_+-=[]{}|;:,.<>?";

// Combined full character pool
static const char ALL_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "!@#$%^&*()_+-=[]{}|;:,.<>?";

// Xorshift128+ 128-bit state
static uint64_t prng_state[2] = { 0x8a5cd789635d2dffULL, 0x121fd2155c472f96ULL };

// Hardware entropy initialization
void password_gen_init_entropy(uint32_t adc_noise_value, uint32_t time_us, uint32_t cycle_count) {
    uint32_t uid[3] = { 0x55AA55AA, 0x12345678, 0x9ABCDEF0 };

#if defined(__arm__) || defined(ARDUINO_ARCH_STM32)
    // Read STM32 96-bit Unique Device ID (factory programmed register)
    volatile uint32_t *stm32_uid = (volatile uint32_t *)0x1FFFF7E8;
    uid[0] = stm32_uid[0];
    uid[1] = stm32_uid[1];
    uid[2] = stm32_uid[2];
#endif

    // Mix hardware entropy into 128-bit PRNG state
    prng_state[0] ^= ((uint64_t)(uid[0] ^ cycle_count) << 32) | (uid[1] ^ time_us);
    prng_state[1] ^= ((uint64_t)(uid[2] ^ adc_noise_value) << 32) | (cycle_count * 2654435761ULL);

    // Warm-up cycles to diffuse initial state
    for (int i = 0; i < 16; ++i) {
        uint64_t s1 = prng_state[0];
        const uint64_t s0 = prng_state[1];
        prng_state[0] = s0;
        s1 ^= s1 << 23;
        prng_state[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    }
}

// Xorshift128+ random generator step
static uint64_t next_random64(void) {
    uint64_t s1 = prng_state[0];
    const uint64_t s0 = prng_state[1];
    prng_state[0] = s0;
    s1 ^= s1 << 23;
    prng_state[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return prng_state[1] + s0;
}

// Random integer in range [0, max_exclusive - 1]
static uint32_t get_random_index(uint32_t max_exclusive) {
    if (max_exclusive <= 1) return 0;
    return (uint32_t)(next_random64() % max_exclusive);
}

void generate_random_password(char *output, size_t length) {
    if (!output) return;

    if (length < MIN_PASSWORD_LEN) length = MIN_PASSWORD_LEN;
    if (length > MAX_PASSWORD_LEN) length = MAX_PASSWORD_LEN;

    // Step 1: Ensure at least one character from each required category
    output[0] = UPPERCASE_CHARS[get_random_index(sizeof(UPPERCASE_CHARS) - 1)];
    output[1] = LOWERCASE_CHARS[get_random_index(sizeof(LOWERCASE_CHARS) - 1)];
    output[2] = DIGIT_CHARS[get_random_index(sizeof(DIGIT_CHARS) - 1)];
    output[3] = SYMBOL_CHARS[get_random_index(sizeof(SYMBOL_CHARS) - 1)];

    // Step 2: Fill remaining positions from full character pool
    size_t all_len = sizeof(ALL_CHARS) - 1;
    for (size_t i = 4; i < length; ++i) {
        output[i] = ALL_CHARS[get_random_index(all_len)];
    }

    // Step 3: Fisher-Yates shuffle so fixed categories are randomly distributed
    for (size_t i = length - 1; i > 0; --i) {
        size_t j = get_random_index(i + 1);
        char temp = output[i];
        output[i] = output[j];
        output[j] = temp;
    }

    output[length] = '\0'; // Null terminator
}
