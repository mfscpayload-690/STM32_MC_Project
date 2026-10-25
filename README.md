# KeyPill: Secure Hardware Password Generator & Auto-Filler

**Hardware-Based Credential Manager using STM32 Blue Pill (STM32F103C8T6)**

---

## 📌 Project Overview

**KeyPill** is an offline, air-gapped hardware password generator and auto-filler built using the **STM32F103C8T6 (ARM Cortex-M3)** microcontroller. 

Operating through an intuitive **0.96" 128×64 I2C OLED display** and **EC11 rotary encoder**, KeyPill allows you to:
1. **Interactive Menu Navigation:** Browse options and view real-time feedback directly on the OLED screen.
2. **Entropy Harvesting:** Gathers physical entropy from microsecond SysTick timers, floating ADC noise (`PA3`), and the factory-programmed 96-bit STM32 Unique Device ID.
3. **Password Generation:** Generates a strong random password (configurable 12, 14, or 16 characters) guaranteeing a mix of uppercase, lowercase, numbers, and symbols.
4. **AES-128 Encryption:** Encrypts credentials in volatile SRAM using AES-128 (NIST FIPS 197).
5. **Flash Storage:** Stores the encrypted ciphertext into the STM32's non-volatile internal flash memory.
6. **Decryption & Auto-Fill:** Decrypts the password and types it directly into the active field on your PC via **USB HID (Human Interface Device) keyboard emulation**, followed by an `ENTER` keystroke.
7. **On-Screen Credential Inspection:** Displays the decrypted password on the OLED with a 6-second live countdown bar before automatic memory scrubbing.
8. **SRAM Scrubbing:** Securely zeros out plaintext password buffers from RAM.

---

## 🔒 Security Advantages

| Attack Vector | Traditional Method / Software | KeyPill Hardware Token |
| :--- | :--- | :--- |
| **Software Keyloggers** | ❌ Vulnerable to OS-level keystroke sniffing | ✅ Emulates hardware USB keyboard directly; generated offline |
| **Weak / Reused Passwords** | ❌ Humans choose memorable, predictable strings | ✅ Enforces high-entropy 12–16 char mixed cryptographic strings |
| **Cloud Leaks / Breaches** | ❌ Databases can be breached online | ✅ 100% offline, air-gapped device without internet or Bluetooth |
| **Flash Memory Extraction** | ❌ Plaintext in memory can be dumped | ✅ Stored in Flash as AES-128 encrypted ciphertext |

---

## 🛠️ Required Hardware Components

| Component | Specifications | Quantity | Purpose |
| :--- | :--- | :---: | :--- |
| **STM32 Blue Pill** | STM32F103C8T6 (ARM Cortex-M3, 72 MHz) | 1 pc | Core microcontroller unit |
| **ST-Link V2** | SWD In-Circuit Debugger / Programmer | 1 pc | Firmware flashing & debugging |
| **OLED Display** | 0.96" 128×64 I2C Module (SSD1306) | 1 pc | Real-time UI, status, & password view |
| **Rotary Encoder** | EC11 Quadrature Encoder Module + Knob Cap | 1 pc | Interactive menu navigation & selection |
| **Status LEDs** | 5mm Standard Red (2 pcs), Green (2 pcs) | 4 pcs | Success/Typing (Green), Busy/Encrypting (Red) |
| **Resistors** | 220 Ω (4 pcs), 10 kΩ pull-ups (5 pcs) | 9 pcs | Current limiting & GPIO pull-ups |
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

### 3. EC11 Rotary Encoder Module (Circular 4/5-Pin Module)
The circular EC11 module provides quadrature rotation (`S1`, `S2`) and an integrated push button (`Key`).

| EC11 Pin Label | STM32 Blue Pill Pin | Description / Function |
| :---: | :---: | :--- |
| **`GND`** | **`GND`** | Common Ground |
| **`S1`** | **`PA1`** | Quadrature Channel A / Clock (`CLK`) |
| **`S2`** | **`PA2`** | Quadrature Channel B / Data (`DT`) |
| **`Key`** | **`PA0`** | Built-in push switch (Click knob) |
| **`5V`** *(Optional)* | **`3.3V`** *(or Unconnected)* | Power rail for on-board SMD pull-up resistors |

> [!TIP]
> **4-Wire Connection (Recommended):** The firmware configures internal pull-up resistors (`INPUT_PULLUP`) on pins `PA0`, `PA1`, and `PA2`. Therefore, you only need to connect **4 wires** (`GND`, `S1`, `S2`, `Key`). The `5V` pin on the module can be left unconnected. If you choose to connect the `5V` pin, connect it to the **3.3V** rail of the Blue Pill (never connect to 5V!).

---

### 4. Status LEDs (with 220 Ω Resistors)
| LED | STM32 Pin | Series Resistor | Function |
| :---: | :---: | :---: | :--- |
| **Green LED** | **`PA8`** | 220 Ω to GND | Lights up on successful USB typing / ready |
| **Red LED** | **`PA9`** | 220 Ω to GND | Lights up during AES encryption & flash write |
| **Onboard LED** | **`PC13`** | Internal | Activity heartbeat (Active LOW) |

---

### 5. Physical Entropy Source
| Peripheral | STM32 Pin | Connection | Function |
| :---: | :---: | :---: | :--- |
| **Floating ADC** | **`PA3`** | Unconnected / Floating | Harvests ambient RF & thermal voltage noise to seed PRNG |

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
7. **Navigate using the Rotary Encoder**:
   - The device displays a splash screen, followed by the main interactive menu on the 128×64 OLED.
   - **Rotate Knob:** Move cursor up/down through the 5 options.
   - **Click Knob (`SW` on PA0):** Execute the highlighted function.

### Interactive Menu Actions:
- **`1. AutoFill New`:** Harvests hardware entropy (PA3 ADC + SysTick + Chip UID), generates a fresh high-entropy password, encrypts via AES-128, writes to internal flash, and automatically types it into the active field on your PC via USB HID + `ENTER`.
- **`2. AutoFill Saved`:** Reads the existing ciphertext from flash, decrypts with AES-128 in SRAM, and types it into the active field + `ENTER`.
- **`3. View Password`:** Decrypts and shows the password directly on the OLED screen with a 6-second live progress bar countdown before auto-scrubbing RAM and clearing the display (or click knob to exit immediately).
- **`4. Set Length`:** Rotate knob to cycle between 12, 14, and 16 characters; click to confirm.
- **`5. System Info`:** Displays MCU frequency (72 MHz), crypto standard (AES-128 NIST), flash storage page, and USB HID status.

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
│   ├── KeyPill.ino                # Main program, menu state machine & USB HID typing
│   ├── aes128.h                   # Standalone AES-128 header
│   ├── aes128.cpp                 # NIST FIPS 197 AES-128 implementation
│   ├── password_gen.h             # Password generator header
│   ├── password_gen.cpp           # Hardware-entropy PRNG & Fisher-Yates generator
│   ├── storage.h                  # Flash/EEPROM memory manager header
│   ├── storage.cpp                # Flash/EEPROM storage implementation
│   ├── oled_ssd1306.h             # Self-contained 128x64 I2C SSD1306 OLED driver
│   ├── oled_ssd1306.cpp           # OLED graphics primitives & text renderer
│   ├── rotary_encoder.h           # EC11 rotary encoder header with debounce
│   └── rotary_encoder.cpp         # Quadrature decoder & switch state machine
├── docs/
│   ├── abstract.tex               # 2-Page Project Abstract (LaTeX)
│   ├── setup_guide.md             # Step-by-step beginner setup guide
│   └── technical_spec.md          # Full technical & cryptographic specification
├── tests/
│   └── test_runner.cpp            # Host C++ test suite (NIST vector validation)
├── Makefile                       # Test automation Makefile
└── README.md                      # Project documentation
```
