#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../KeyPill/aes128.h"
#include "../KeyPill/password_gen.h"
#include "../KeyPill/storage.h"

// NIST FIPS 197 Appendix B Test Vector for AES-128
// Key: 2b7e151628aed2a6abf7158809cf4f3c
// Plaintext: 6bc1bee22e409f96e93d7e117393172a
// Ciphertext: 3ad77bb40d7a3660a89ecaf32466ef97
void test_nist_aes128(void) {
    printf("[*] Running NIST FIPS 197 AES-128 Vector Test...\n");
    const uint8_t key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    const uint8_t plaintext[16] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    };
    const uint8_t expected_cipher[16] = {
        0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
        0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
    };

    uint8_t cipher[16];
    aes128_encrypt_block(plaintext, key, cipher);
    assert(memcmp(cipher, expected_cipher, 16) == 0);

    uint8_t decrypted[16];
    aes128_decrypt_block(cipher, key, decrypted);
    assert(memcmp(decrypted, plaintext, 16) == 0);
    printf("    [PASS] AES-128 Block Encryption & Decryption matches NIST vector!\n");
}

void test_padded_encryption(void) {
    printf("[*] Running AES-128 PKCS#7 Padded Password Encryption Test...\n");
    const uint8_t key[16] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };

    const char *test_passwords[] = {
        "A1b!C2d@E3f#",          // 12 chars
        "Pass123!@#Test",        // 14 chars
        "VeryStr0ng!P@ss",       // 15 chars
        "Max16Chars!Pass"        // 16 chars
    };

    for (int t = 0; t < 4; ++t) {
        const char *pass = test_passwords[t];
        size_t pass_len = strlen(pass);
        uint8_t encrypted[32];
        size_t enc_len = 0;

        aes128_encrypt_padded((const uint8_t *)pass, pass_len, key, encrypted, &enc_len);
        assert(enc_len == 16 || enc_len == 32);

        char decrypted[32];
        size_t dec_len = 0;
        int ok = aes128_decrypt_padded(encrypted, enc_len, key, (uint8_t *)decrypted, &dec_len);
        assert(ok == 1);
        assert(dec_len == pass_len);
        assert(strcmp(pass, decrypted) == 0);
    }
    printf("    [PASS] Padded password encryption & decryption successful for all lengths!\n");
}

void test_password_generation(void) {
    printf("[*] Running Password Generator Randomness & Character Set Test...\n");
    password_gen_init_entropy(1234, 567890, 72000);

    for (int i = 0; i < 50; ++i) {
        char pass[32];
        generate_random_password(pass, 14);
        assert(strlen(pass) == 14);

        int has_upper = 0, has_lower = 0, has_digit = 0, has_symbol = 0;
        for (size_t j = 0; j < 14; ++j) {
            char c = pass[j];
            if (c >= 'A' && c <= 'Z') has_upper = 1;
            else if (c >= 'a' && c <= 'z') has_lower = 1;
            else if (c >= '0' && c <= '9') has_digit = 1;
            else has_symbol = 1;
        }

        assert(has_upper == 1);
        assert(has_lower == 1);
        assert(has_digit == 1);
        assert(has_symbol == 1);
    }
    printf("    [PASS] 50 passwords generated; 100%% met upper/lower/digit/symbol requirement!\n");
}

void test_storage_pipeline(void) {
    printf("[*] Running Storage Save & Load Integrity Test...\n");
    storage_init();

    struct PasswordRecord rec;
    rec.magic = STORAGE_MAGIC;
    rec.password_len = 14;
    rec.encrypted_len = 16;
    memset(rec.encrypted_data, 0xAA, 16);
    rec.checksum = storage_compute_checksum(&rec);

    int saved = storage_save_record(&rec);
    assert(saved == 1);

    struct PasswordRecord loaded;
    int loaded_ok = storage_load_record(&loaded);
    assert(loaded_ok == 1);
    assert(loaded.magic == STORAGE_MAGIC);
    assert(loaded.password_len == 14);
    assert(loaded.encrypted_len == 16);
    assert(memcmp(loaded.encrypted_data, rec.encrypted_data, 16) == 0);
    assert(loaded.checksum == rec.checksum);

    printf("    [PASS] Storage integrity verified!\n");
}

int main(void) {
    printf("====================================================\n");
    printf("  KeyPill Unit Test Runner (Host-side validation)  \n");
    printf("====================================================\n");

    test_nist_aes128();
    test_padded_encryption();
    test_password_generation();
    test_storage_pipeline();

    printf("\n>>> ALL TESTS PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
