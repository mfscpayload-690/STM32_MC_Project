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

## 🛠️ Required Hardware Components

| Component | Specifications | Quantity | Purpose |
| :--- | :--- | :---: | :--- |
| **STM32 Blue Pill** | STM32F103C8T6 (ARM Cortex-M3, 72 MHz) | 1 pc | Core microcontroller unit |
| **ST-Link V2** | SWD In-Circuit Debugger / Programmer | 1 pc | Firmware flashing & debugging |
| **OLED Display** | 0.96" 128×64 I2C Module (SSD1306) | 1 pc | Real-time UI, status, & password view |
| **Rotary Encoder** | KY-040 Quadrature Encoder + Push Switch | 1 pc | Interactive menu navigation & selection |
| **Status LEDs** | 5mm Standard Green (1 pc), Red (1 pc) | 2 pcs | Success/Typing (Green), Busy/Encrypting (Red) |
| **Resistors** | 220 Ω (for LEDs), 10 kΩ pull-ups | 4 pcs | Current limiting & GPIO pull-ups |
| **Breadboard & Wires** | Solderless 830-pt + DuPont Jumpers (M-M, M-F) | 1 set | Prototyping interconnections |
| **USB Micro-B Cable** | High-speed data cable | 1 pc | USB HID Keyboard connection to PC |

---

## 🔌 Hardware Wiring & Pinout

### 1. ST-Link V2 to STM32 Blue Pill (4-Wire SWD)
| STM32 Blue Pill Pin | ST-Link V2 Pin | Description |
| :---: | :---: | :--- |
| **`3V3`** | **`3.3V`** (Pin 7 or 8) | 3.3V Power Supply (Never connect 5V to 3V3!) |
| **`SWIO`** | **`SWDIO`** (Pin 4) | Serial Wire Debug Data Line |
| **`SWCLK`** | **`SWCLK`** (Pin 2) | Serial Wire Debug Clock Line |
| **`GND`** | **`GND`** (Pin 5 or 6) | Common Ground |

---

### 2. 0.96" I2C OLED Display (SSD1306)
| OLED Pin | STM32 Blue Pill Pin | Description |
| :---: | :---: | :--- |
| **`GND`** | **`GND`** | Ground |
| **`VCC`** | **`3.3V`** | 3.3V Power |
| **`SCL`** | **`PB6`** | Hardware I2C1 Clock |
| **`SDA`** | **`PB7`** | Hardware I2C1 Data |

---

### 3. KY-040 Rotary Encoder Module
| KY-040 Pin | STM32 Blue Pill Pin | Description |
| :---: | :---: | :--- |
| **`GND`** | **`GND`** | Ground |
| **`+` (VCC)** | **`3.3V`** | 3.3V Power |
| **`SW` (Button)** | **`PA0`** | Built-in push switch (Click knob) |
| **`CLK` (A)** | **`PA1`** | Quadrature Clock output |
| **`DT` (B)** | **`PA2`** | Quadrature Data output |

---

### 4. Status LEDs (with 220 Ω Resistors)
| LED | STM32 Pin | Series Resistor | Function |
| :---: | :---: | :---: | :--- |
| **Green LED** | **`PA8`** | 220 Ω to GND | Lights up on successful USB typing / ready |
| **Red LED** | **`PA9`** | 220 Ω to GND | Lights up during AES encryption & flash write |
| **Onboard LED** | **`PC13`** | Internal | Activity heartbeat (Active LOW) |

---

## 🕹️ Interactive Menu Navigation

The 128×64 OLED screen displays a clean menu operated entirely with the rotary encoder:
* **Rotate Knob (CW / CCW):** Scroll through menu items or adjust values.
* **Click Knob (`SW`):** Select menu item / Confirm action.

```text
+------------------------+
|  === KEYPILL v1.0 ===  |
| > 1. AutoFill New      |
|   2. AutoFill Saved    |
|   3. View Password     |
|   4. Set Length: 14    |
|   5. System Info       |
+------------------------+
```

1. **AutoFill New:** Generates strong random password, encrypts via AES-128, saves to flash, types to host PC via USB HID, and presses `ENTER`.
2. **AutoFill Saved:** Decrypts existing password from Flash and types into PC.
3. **View Password:** Displays decrypted password on OLED for 6 seconds with a live countdown progress bar, then clears it securely.
4. **Set Length:** Rotate knob to configure password length (12, 14, or 16 characters).
5. **System Info:** Displays MCU clock (72MHz), crypto standard, Flash page, and USB status.

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
