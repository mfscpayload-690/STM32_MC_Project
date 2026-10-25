# Technical & Cryptographic Specification

## 1. System Architecture

The KeyPill architecture consists of decoupled firmware modules executing on the ARM Cortex-M3 core:

```
+-------------------------------------------------------------------------+
|                  USER INTERFACE & NAVIGATION SUBSYSTEM                  |
|  - EC11 Quadrature Rotary Encoder (PA1 S1/CLK, PA2 S2/DT)               |
|  - Push Key Selection Button (PA0 Key/SW, Active LOW, Internal Pull-Up)  |
|  - 0.96" 128x64 SSD1306 OLED Display (PB6 SCL, PB7 SDA @ 400kHz I2C)    |
|  - Visual Indicators: Green (PA8 Typing), Red (PA9 Busy), Heartbeat PC13|
+------------------------------------+------------------------------------+
                                     | Interactive Menu Selection
                                     v
+-------------------------------------------------------------------------+
|                        ENTROPY HARVESTING & PRNG                        |
|  - SysTick microsecond counter (VAL jitter on user click)               |
|  - Floating ADC noise channel (PA3 thermal/RF voltage noise)            |
|  - STM32 96-bit factory Unique Device ID (0x1FFFF7E8 register)          |
|  - Xorshift128+ state engine -> Fisher-Yates character distribution     |
+------------------------------------+------------------------------------+
                                     | Plaintext Password (12, 14, or 16 chars)
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
|  - STM32 Emulated EEPROM (Top Flash Page, e.g. Page 63 @ 0x0800FC00)    |
|  - Record header: Magic (0x4B50494C), Lengths, Checksum (44 bytes total)|
+------------------------------------+------------------------------------+
                                     | Plaintext Typed
                                     v
+-------------------------------------------------------------------------+
|                       USB HID KEYBOARD SUBSYSTEM                        |
|  - USB 2.0 Full Speed (12 Mbps) Device Controller                       |
|  - HID Class Report Protocol (0x01) with hardware scan codes            |
|  - Inter-keystroke debounce delay (12ms) + automatic RETURN keystroke   |
+------------------------------------+------------------------------------+
                                     | SRAM Scrubbing
                                     v
+-------------------------------------------------------------------------+
|                        SECURE MEMORY PURGE                              |
|  - Volatile zeroing of plaintext password buffers & decrypted structures|
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
- **Display Module:** 0.96" SSD1306 monochrome 128×64 OLED over hardware I2C1 (PB6/PB7)
- **Navigation Module:** EC11 rotary quadrature encoder module with integrated tactile push key
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
2. **Analog ADC Noise (`PA3`):** Floating ADC input channel measures ambient electromagnetic interference and thermal voltage fluctuations.
3. **Silicon Factory Unique ID:** Every STM32 chip contains a factory-programmed 96-bit unique serial number stored at memory address `0x1FFFF7E8` (3 words: `UID[0]`, `UID[1]`, `UID[2]`).

### PRNG Algorithm: Xorshift128+
- State size: 128 bits (`uint64_t s[2]`).
- Period: $2^{128} - 1$.
- Extremely fast on ARM Cortex-M3 (bitwise shifts and XOR operations).
- Shuffling via **Fisher-Yates** algorithm eliminates selection bias and guarantees uniform distribution across character categories (uppercase, lowercase, digits, symbols).

---

## 5. Non-Volatile Storage Layout

Encrypted passwords are saved in the top flash page of the STM32F103 (Page 63, `0x0800FC00` on 64KB chips) via the STM32duino Flash-emulated EEPROM subsystem:

| Offset | Field | Size | Description |
| :---: | :--- | :---: | :--- |
| `0x00` | `magic` | 4 bytes | Magic identifier: `0x4B50494C` ("KPIL") |
| `0x04` | `password_len` | 1 byte | Plaintext password length (12, 14, or 16) |
| `0x05` | `encrypted_len` | 1 byte | Ciphertext length (16 or 32 bytes) |
| `0x06` | `encrypted_data` | 32 bytes | AES-128 ciphertext payload |
| `0x26` | *(padding)* | 2 bytes | Compiler boundary alignment to 32-bit word |
| `0x28` | `checksum` | 4 bytes | Additive polynomial checksum |

**Total Record Size:** 44 bytes.

---

## 6. USB HID Keyboard Protocol

The device uses standard USB HID Keyboard Class Specification (Usage Page `0x07`):
- **Modifiers:** Standard Shift, Ctrl, Alt handling.
- **Scan Code Transmission:** ASCII characters are translated into HID usage scan codes.
- **Inter-keystroke Delay:** 12 ms delay between key down and key up ensures cross-platform compatibility across Windows, Linux, macOS, and mobile devices over OTG.
