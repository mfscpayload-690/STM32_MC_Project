/**
 * @file password_gen.h
 * @brief High-entropy cryptographic random password generator for STM32 Blue Pill
 */

#ifndef PASSWORD_GEN_H
#define PASSWORD_GEN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_PASSWORD_LEN 14 // Default 14 characters (between 12 and 16)
#define MIN_PASSWORD_LEN     12
#define MAX_PASSWORD_LEN     16

/**
 * @brief Initialize the PRNG with hardware entropy (SysTick, Floating ADC, STM32 UID).
 * @param adc_noise_value Value read from floating ADC pin (e.g. analogRead(PA3)).
 * @param time_us Current microsecond timestamp (micros()).
 * @param cycle_count Current SysTick value.
 */
void password_gen_init_entropy(uint32_t adc_noise_value, uint32_t time_us, uint32_t cycle_count);

/**
 * @brief Generates a cryptographically strong random password.
 *        Guarantees inclusion of uppercase, lowercase, numbers, and special symbols.
 * @param output Destination buffer for password string (must be at least length + 1 bytes).
 * @param length Desired password length (clamped to 12 - 16 chars).
 */
void generate_random_password(char *output, size_t length);

#ifdef __cplusplus
}
#endif

#endif // PASSWORD_GEN_H
