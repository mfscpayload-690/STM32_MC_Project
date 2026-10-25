# Step-by-Step Hardware & Software Setup Guide

This guide walks you through setting up the **STM32 Blue Pill**, installing the required Arduino IDE cores and ST-Link drivers, wiring the breadboard circuit, and flashing the firmware.

---

## 1. Software & Driver Installation

### Step 1: Install Arduino IDE
1. Download Arduino IDE (v2.x or v1.8.19) from [arduino.cc](https://www.arduino.cc/en/software).
2. Follow standard installer steps for your operating system.

### Step 2: Install ST-Link USB Driver
The ST-Link V2 USB programmer requires a driver for your computer to recognize it:
- **Windows:**
  1. Download [STSW-LINK009 Driver from STMicroelectronics](https://www.st.com/en/development-tools/stsw-link009.html) (free download after entering email).
  2. Extract the zip file and run `dpinst_amd64.exe` (or `dpinst_x86.exe` on 32-bit systems).
  3. When plugged in, the ST-Link V2 will show up in Device Manager under **Universal Serial Bus devices** as *STMicroelectronics STLink dongle*.
- **Linux:**
  - Copy the bundled OpenOCD udev rules for ST-Link access:
    ```bash
    sudo cp ~/.arduino15/packages/STMicroelectronics/tools/xpack-openocd/0.12.0-6/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules
    sudo udevadm trigger
    ```
  - Unplug and replug the ST-Link V2 USB dongle.

### Step 3: Add STM32 Core to Arduino IDE
1. Open Arduino IDE.
2. Go to **File -> Preferences** (macOS: **Arduino IDE -> Settings**).
3. In the field labeled **Additional boards manager URLs**, enter:
   ```
   https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
   ```
4. Click **OK**.
5. Open the Boards Manager (**Tools -> Board -> Boards Manager...**).
6. Type `STM32` into the search bar.
7. Click **Install** on **STM32 MCU based boards** by *STMicroelectronics*.

---

## 2. Hardware Wiring

### Step 1: Connecting ST-Link V2 to STM32 Blue Pill (SWD)
Connect the 4 header pins at the bottom edge of the Blue Pill to the ST-Link V2 using DuPont jumper wires:

| Blue Pill Pin Label | ST-Link V2 Pin | Description |
| :---: | :---: | :--- |
| **`3V3`** | **`3.3V`** | 3.3V Power Supply (DO NOT connect to 5V!) |
| **`SWIO` (or `IO`)** | **`SWDIO`** | Serial Wire Debug Data Line |
| **`SWCLK` (or `CLK`)** | **`SWCLK`** | Serial Wire Debug Clock Line |
| **`GND`** | **`GND`** | Common Ground |

> [!WARNING]
> Verify the orientation of the 4-pin header on your Blue Pill. Connecting 3.3V to GND can damage the microcontroller or programmer.

---

### Step 2: Connecting the 0.96" I2C OLED Display (SSD1306)
Connect the 4 pins of the OLED display to the Blue Pill:

| OLED Pin | Blue Pill Pin | Description |
| :---: | :---: | :--- |
| **`GND`** | **`GND`** | Ground |
| **`VCC`** | **`3.3V`** | 3.3V Power Rail |
| **`SCL`** | **`PB6`** | Hardware I2C1 Clock Line |
| **`SDA`** | **`PB7`** | Hardware I2C1 Data Line |

---

### Step 3: Connecting the EC11 Rotary Encoder Module (Circular 4/5-Pin Module)
The circular EC11 module provides quadrature signals (`S1`, `S2`), an integrated push button (`Key`), and power rails:

| EC11 Pin Label | Blue Pill Pin | Description / Function |
| :---: | :---: | :--- |
| **`GND`** | **`GND`** | Ground |
| **`S1`** | **`PA1`** | Quadrature Clock / Phase A (`CLK`) |
| **`S2`** | **`PA2`** | Quadrature Data / Phase B (`DT`) |
| **`Key`** | **`PA0`** | Built-in push switch (Knob click / `SW`) |
| **`5V`** *(Optional)* | **`3.3V`** *(or leave unconnected)* | Power rail for on-board SMD pull-up resistors |

> [!TIP]
> **4-Wire Connection (Recommended):** The STM32 Blue Pill firmware enables internal pull-ups (`INPUT_PULLUP`) on pins `PA0`, `PA1`, and `PA2`. You only need to wire **4 pins**: **`GND`**, **`S1`**, **`S2`**, and **`Key`**. The **`5V`** pin can remain unconnected. If you choose to connect the `5V` pin, connect it to the Blue Pill's **3.3V** pin (do not connect to 5V).

---

### Step 4: Connecting the Status LEDs
Connect the two external 5mm LEDs on your breadboard:
* **Green LED (Success / Typing):**
  - Anode (Long leg) $\rightarrow$ Blue Pill pin **`PA8`**.
  - Cathode (Short leg) $\rightarrow$ In series with a **220 Ω resistor** to **`GND`**.
* **Red LED (Encrypting / Busy / Alert):**
  - Anode (Long leg) $\rightarrow$ Blue Pill pin **`PA9`**.
  - Cathode (Short leg) $\rightarrow$ In series with a **220 Ω resistor** to **`GND`**.

---

### Step 5: Physical Entropy Harvesting Pin
Leave pin **`PA3`** unconnected (floating) on the breadboard. It acts as an antenna capturing ambient electromagnetic and thermal noise through the internal ADC, seeding the PRNG with physical randomness.

---

### Step 6: Dual USB Cables Setup
For proper operation and testing:
1. **ST-Link V2 USB:** Plugs into PC for programming and power.
2. **Blue Pill Micro-B USB Cable:** Plugs from the Blue Pill's on-board Micro-USB port into a second USB port on your PC. This connection acts as the **USB HID Keyboard**!

---

## 3. Configuring Arduino IDE Settings

Open `KeyPill/KeyPill.ino` in Arduino IDE. Under the **Tools** menu, configure:

- **Board:** `Generic STM32F1 series`
- **Board part number:** `BluePill F103C8`
- **U(S)ART support:** `Enabled (generic 'Serial')`
- **USB support (if available):** `HID (keyboard/mouse)`
- **USB speed:** `Low/Full Speed`
- **Upload method:** `STM32CubeProgrammer (SWD)` or `STLink`

---

## 4. Flashing Firmware

1. Verify the board settings in the Tools menu.
2. Make sure the yellow jumpers on the Blue Pill (**BOOT0** and **BOOT1**) are both set to **0** (default operating mode).
3. Click the **Upload** button (`Ctrl + U` or arrow icon in the top left).
4. The output console should display:
   ```
   Download in progress ...
   File download complete
   Time elapsed...
   GO ...
   ```
5. The device displays a startup splash screen on the OLED: `"KEYPILL - Hardware Password Token - STM32 + AES-128"`.

---

## 5. Live Demonstration & Verification

1. Open a blank text editor like **Notepad** (Windows), **TextEdit** (macOS), or a browser login box.
2. Place the typing cursor into the password field.
3. Observe the 128×64 OLED screen displaying the 5-item main menu:
   - `> 1. AutoFill New`
   - `  2. AutoFill Saved`
   - `  3. View Password`
   - `  4. Set Length: 14`
   - `  5. System Info`
4. **Rotate the Rotary Knob (CW / CCW):**
   - The selection cursor moves dynamically between the 5 menu items.
5. **Click the Rotary Knob (SW on PA0):**
   - **`1. AutoFill New`:** Microcontroller gathers entropy, generates a fresh random password, encrypts it with AES-128, saves it into internal flash, types it into your PC, and presses `ENTER`. Status LEDs and OLED reflect real-time progress.
   - **`2. AutoFill Saved`:** Decrypts the existing password from flash and types it directly into the PC + `ENTER`.
   - **`3. View Password`:** Shows the stored password on the OLED with a live 6-second auto-clearing progress bar for secure visual inspection.
   - **`4. Set Length`:** Rotate knob to adjust password length (12, 14, or 16 characters) and click to save.
   - **`5. System Info`:** Displays microcontroller frequency (72 MHz), NIST crypto mode, flash page, and USB HID status.

---

## 6. Troubleshooting Common Issues

### Issue 1: "No target connected" or "ST-Link error"
- **Solution:** 
  1. Check the 4 DuPont wires between ST-Link and Blue Pill.
  2. Press and hold the reset button on the Blue Pill, click Upload in Arduino IDE, and release the reset button as soon as "Connecting..." appears.
  3. Ensure both yellow jumpers (BOOT0, BOOT1) are set to position `0`.

### Issue 2: Computer doesn't detect USB Keyboard
- **Solution:** 
  1. Ensure **Tools -> USB support** is set to **`HID (keyboard/mouse)`**.
  2. The sketch includes automatic USB software disconnect (`trigger_usb_reenumeration()`) on pin PA12. Make sure your Micro-USB cable supports data lines (not a charge-only cable).
