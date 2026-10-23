# KeyPill: Secure Hardware Password Generator & Auto-Filler

**Hardware-Based Credential Manager using STM32 Blue Pill (STM32F103C8T6)**

---

## 📌 Project Overview

**KeyPill** is an offline, air-gapped hardware password generator and auto-filler built using the **STM32F103C8T6 (ARM Cortex-M3)** microcontroller. 

When you press the physical button on the device:
1. **Entropy Harvesting:** Gathers physical entropy from microsecond SysTick timers, floating ADC noise, and the factory-programmed 96-bit STM32 Unique Device ID.
2. **Password Generation:** Generates a strong 14-character random password guaranteeing a mix of uppercase, lowercase, numbers, and symbols.
3. **AES-128 Encryption:** Encrypts the password immediately in volatile SRAM using AES-128 (NIST FIPS 197).
4. **Flash Storage:** Stores the encrypted ciphertext into the STM32's non-volatile internal flash memory.
5. **Decryption & Auto-Fill:** Decrypts the password and types it directly into the active field on your PC via **USB HID (Human Interface Device) keyboard emulation**, followed by an `ENTER` keystroke.
6. **SRAM Scrubbing:** Securely zeros out plaintext password buffers from RAM.

---

## 🔒 Security Advantages

| Attack Vector | Traditional Method / Software | KeyPill Hardware Token |
| :--- | :--- | :--- |
| **Software Keyloggers** | ❌ Vulnerable to OS-level keystroke sniffing | ✅ Emulates hardware USB keyboard directly; generated offline |
| **Weak / Reused Passwords** | ❌ Humans choose memorable, predictable strings | ✅ Enforces high-entropy 14-char mixed cryptographic strings |
| **Cloud Leaks / Breaches** | ❌ Databases can be breached online | ✅ 100% offline, air-gapped device without internet or Bluetooth |
| **Flash Memory Extraction** | ❌ Plaintext in memory can be dumped | ✅ Stored in Flash as AES-128 encrypted ciphertext |

---

## 🛠️ Required Hardware

| Component | Quantity | Purpose |
| :--- | :---: | :--- |
| **STM32 Blue Pill Board** | 1 | Microcontroller (STM32F103C8T6, 72 MHz, 64 KB Flash) |
| **ST-Link V2 Programmer** | 1 | SWD In-Circuit Debugger & Firmware Flasher |
| **Breadboard** | 1 | Circuit prototyping |
| **Momentary Push Button** | 1 | Trigger button for password generation/filling |
| **10 kΩ Resistor** | 1 | Pull-down resistor for button (PA0) |
| **USB Micro-B Cable** | 1 | USB data connection between Blue Pill and PC |
| **DuPont Jumper Wires** | Several | Interconnections (SWD & breadboard) |

---

## 🔌 Hardware Wiring & Pinout

### 1. ST-Link V2 to STM32 Blue Pill (SWD 4-Wire Interface)
Connect the 4 header pins at the bottom edge of the Blue Pill to the ST-Link V2:

| STM32 Blue Pill Pin | ST-Link V2 Pin | Wire Color (Standard) | Description |
| :---: | :---: | :---: | :--- |
| **`3V3`** | **`3.3V`** (Pin 7 or 8) | Red | 3.3V Logic & Power |
| **`SWDIO`** | **`SWDIO`** (Pin 4) | Yellow | Serial Wire Data Input/Output |
| **`SWCLK`** | **`SWCLK`** (Pin 2) | Green | Serial Wire Clock |
| **`GND`** | **`GND`** (Pin 5 or 6) | Blue / Black | Ground |

> [!IMPORTANT]
> Always power the Blue Pill using **3.3V** from ST-Link, **NEVER 5V** onto the 3V3 pin!

---

### 2. Breadboard Button Circuit (PA0)
Connect the push button to pin **PA0** with a pull-down resistor:
```
           +3.3V
             |
           [Button]
             |
 PA0 --------+
             |
          [10kΩ]
             |
            GND
```
- One side of the button goes to **3.3V**.
- The other side connects to **PA0** AND to one leg of the **10kΩ resistor**.
- The second leg of the **10kΩ resistor** connects to **GND**.

---

## 💻 Software & Driver Setup

### 1. Install Arduino IDE
Download and install **Arduino IDE 2.x** or **1.8.x** from [arduino.cc](https://www.arduino.cc/en/software).

### 2. Install ST-Link USB Driver
- **Windows:** Download and install the official [STSW-LINK009 Driver](https://www.st.com/en/development-tools/stsw-link009.html).
- **Linux:** Install `stlink-tools` via package manager (e.g., `sudo apt install stlink-tools` or `sudo pacman -S stlink`).

### 3. Add STM32 Board Support to Arduino IDE
1. Open Arduino IDE, go to **File -> Preferences** (`Ctrl + ,`).
2. In **Additional Boards Manager URLs**, paste:
   ```
   https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
   ```
3. Click **OK**.
4. Go to **Tools -> Board -> Boards Manager...**.
5. Search for **STM32** and install **STM32 MCU based boards** by *STMicroelectronics*.

---

## ⚙️ Arduino IDE Configuration

Go to the **Tools** menu in Arduino IDE and select the following settings:

| Setting | Value |
| :--- | :--- |
| **Board** | `Generic STM32F1 series` |
| **Board part number** | `BluePill F103C8` (or `BluePill F103C8 (64KB)`) |
| **U(S)ART support** | `Enabled (generic 'Serial')` |
| **USB support (if available)** | **`HID (keyboard/mouse)`** *(CRITICAL: enables USB Keyboard!)* |
| **USB speed** | `Low/Full Speed` |
| **Upload method** | **`STM32CubeProgrammer (SWD)`** or **`STLink`** |

> [!NOTE]
> All cryptography (AES-128) and random password generation modules are **self-contained** inside the `KeyPill/` sketch directory. No extra zip libraries need to be searched or installed!

---

## 🚀 How to Upload & Run

1. Connect the **ST-Link V2** to your computer USB port and the 4-pin SWD cable to the Blue Pill.
2. Connect a **USB Micro-B cable** from the Blue Pill's USB port to another USB port on your computer (this carries the USB Keyboard data!).
3. Open `KeyPill/KeyPill.ino` in Arduino IDE.
4. Click the **Upload** arrow button (`Ctrl + U`).
5. Once uploaded, open **Notepad**, **TextEdit**, or any browser login page.
6. Place your cursor into the password field.
7. **Press the button on PA0**:
   - The Blue Pill generates a new password.
   - It encrypts and saves it to internal flash.
   - It automatically types the password into the field and presses **ENTER**!

### Button Control Modes:
- **Short Press (< 2 seconds):** Generates a brand-new random password, saves to flash, and types it.
- **Long Press (> 2 seconds):** Re-types the currently saved password from flash (useful when logging into your existing account again!).

---

## 🧪 Host Unit Testing

You can verify the cryptographic algorithms and password generation logic on any PC without the hardware:
```bash
make test
```
**Output:**
```
====================================================
  KeyPill Unit Test Runner (Host-side validation)  
====================================================
[*] Running NIST FIPS 197 AES-128 Vector Test...
    [PASS] AES-128 Block Encryption & Decryption matches NIST vector!
[*] Running AES-128 PKCS#7 Padded Password Encryption Test...
    [PASS] Padded password encryption & decryption successful for all lengths!
[*] Running Password Generator Randomness & Character Set Test...
    [PASS] 50 passwords generated; 100% met upper/lower/digit/symbol requirement!
[*] Running Storage Save & Load Integrity Test...
    [PASS] Storage integrity verified!

>>> ALL TESTS PASSED SUCCESSFULLY! <<<
```

---

## 📂 Project Repository Structure

```
STM32-bluepill/
├── KeyPill/                       # Main Arduino IDE Sketch Folder
│   ├── KeyPill.ino                # Main program entry, button debounce, USB HID typing
│   ├── aes128.h                   # Standalone AES-128 header
│   ├── aes128.cpp                 # NIST FIPS 197 AES-128 implementation
│   ├── password_gen.h             # Password generator header
│   ├── password_gen.cpp           # Hardware-entropy PRNG & Fisher-Yates generator
│   ├── storage.h                  # Flash/EEPROM memory manager header
│   └── storage.cpp                # Flash/EEPROM storage implementation
├── docs/
│   ├── abstract.tex               # 2-Page Project Abstract (LaTeX)
│   ├── setup_guide.md             # Step-by-step beginner setup guide
│   └── technical_spec.md          # Full technical & cryptographic specification
├── tests/
│   └── test_runner.cpp            # Host C++ test suite (NIST vector validation)
├── Makefile                       # Test automation Makefile
└── README.md                      # Project documentation
```
