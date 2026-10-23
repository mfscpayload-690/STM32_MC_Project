# Technical & Cryptographic Specification

## 1. System Architecture

The KeyPill architecture consists of four decoupled firmware modules executing on the ARM Cortex-M3 core:

```
+-------------------------------------------------------------------------+
|                              USER TRIGGER                               |
|                  (Physical Push Button on GPIO PA0)                     |
+------------------------------------+------------------------------------+
                                     |
                                     v
+-------------------------------------------------------------------------+
|                        ENTROPY HARVESTING & PRNG                        |
|  - SysTick microsecond counter (VAL)                                    |
|  - Floating ADC noise (PA1)                                             |
|  - STM32 96-bit factory Unique Device ID (0x1FFFF7E8)                   |
|  - Xorshift128+ state engine -> Fisher-Yates character distribution     |
+------------------------------------+------------------------------------+
                                     | Plaintext Password (14 chars)
                                     v
+-------------------------------------------------------------------------+
|                         AES-128 CRYPTO ENGINE                           |
|  - NIST FIPS 197 standard                                               |
|  - 10 rounds: SubBytes, ShiftRows, MixColumns, AddRoundKey              |
|  - PKCS#7 block padding (16-byte alignment)                            |
+------------------------------------+------------------------------------+
                                     | Ciphertext (16 or 32 bytes)
                                     v
+------------------------------------+------------------------------------+
|                       INTERNAL FLASH STORAGE                            |
|  - Page 63 (0x0800FC00 - 0x0800FFFF) of STM32F103 Flash                 |
|  - Record header: Magic (0x4B50494C), Lengths, Checksum                 |
+------------------------------------+------------------------------------+
                                     | Decrypted Plaintext
                                     v
+-------------------------------------------------------------------------+
|                       USB HID KEYBOARD SUBSYSTEM                        |
|  - USB 2.0 Full Speed (12 Mbps)                                         |
|  - HID Class Report Protocol (0x01)                                     |
|  - Keystroke translation + 12ms human debounce typing delay             |
+-------------------------------------------------------------------------+
```

---

## 2. Hardware Specifications

- **Microcontroller:** STM32F103C8T6 (Medium-density performance line)
- **Core Architecture:** ARM 32-bit Cortex-M3
- **Clock Speed:** 72 MHz (8 MHz external crystal multiplied via PLL)
- **Flash Memory:** 64 KB (organized into 64 pages of 1 KB each)
- **SRAM:** 20 KB
- **Operating Voltage:** 3.3V DC (onboard regulator accepts 5V from USB)
- **USB Peripheral:** USB 2.0 Full Speed (12 Mbit/s) Device Controller with shared 512-byte packet buffer

---

## 3. Cryptographic Implementation

### AES-128 Block Cipher
- **Standard:** National Institute of Standards and Technology (NIST) FIPS 197
- **Key Length:** 128 bits (16 bytes)
- **Block Size:** 128 bits (16 bytes)
- **Rounds ($N_r$):** 10 rounds
- **Round Key Schedule:** Generates 11 round keys (176 bytes total) using the standard AES Key Expansion algorithm:
  - $\text{RotWord} \rightarrow \text{SubWord} \rightarrow \text{Rcon} \text{ XOR}$.

### Padding Scheme (PKCS#7)
For a password of length $L$ bytes:
- Padding length $P = 16 - (L \pmod{16})$.
- Each padding byte is assigned value $P$.
- Example: A 14-character password has $P = 2$, resulting in two trailing bytes `0x02, 0x02`.
- Total ciphertext size is strictly aligned to 16 bytes.

---

## 4. Hardware Entropy & PRNG

True randomness on microcontrollers without dedicated hardware TRNG is synthesized through multi-source physical entropy:

1. **SysTick Counter (`SysTick->VAL`):** Counts down from 71999 at 72 MHz. Microsecond variations in user human button press timing introduce several bits of high-frequency jitter.
2. **Analog ADC Noise (`PA1`):** Floating ADC input channel measures ambient electromagnetic interference and thermal voltage fluctuations.
3. **Silicon Factory Unique ID:** Every STM32 chip contains a factory-programmed 96-bit unique serial number stored at memory address `0x1FFFF7E8` (3 words: `UID[0]`, `UID[1]`, `UID[2]`).

### PRNG Algorithm: Xorshift128+
- State size: 128 bits (`uint64_t s[2]`).
- Period: $2^{128} - 1$.
- Extremely fast on ARM Cortex-M3 (bitwise shifts and XOR operations).
- Shuffling via **Fisher-Yates** algorithm eliminates selection bias and guarantees uniform distribution.

---

## 5. Non-Volatile Storage Layout

Encrypted passwords are saved in the top flash page of the STM32F103C8 (Page 63, `0x0800FC00`):

| Offset | Field | Size | Description |
| :---: | :--- | :---: | :--- |
| `0x00` | `magic` | 4 bytes | Magic identifier: `0x4B50494C` ("KPIL") |
| `0x04` | `password_len` | 1 byte | Plaintext password length (e.g. 14) |
| `0x05` | `encrypted_len` | 1 byte | Ciphertext length (16 or 32 bytes) |
| `0x06` | `encrypted_data` | 32 bytes | AES-128 ciphertext payload |
| `0x26` | `checksum` | 4 bytes | Additive polynomial checksum |

**Total Record Size:** 42 bytes.

---

## 6. USB HID Keyboard Protocol

The device uses standard USB HID Keyboard Class Specification (Usage Page `0x07`):
- **Modifiers:** Standard Shift, Ctrl, Alt handling.
- **Scan Code Transmission:** ASCII characters are translated into HID usage scan codes.
- **Inter-keystroke Delay:** 12 ms delay between key down and key up ensures cross-platform compatibility across Windows, Linux, macOS, and mobile devices over OTG.
