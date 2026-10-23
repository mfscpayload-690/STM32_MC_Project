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
  - Run: `sudo apt install stlink-tools` (Debian/Ubuntu) or `sudo pacman -S stlink` (Arch).
  - Add udev rules if necessary for non-root access:
    ```bash
    sudo cp /usr/lib/udev/rules.d/49-stlink* /etc/udev/rules.d/
    sudo udevadm control --reload-rules
    ```

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

### Step 1: Connecting ST-Link V2 to STM32 Blue Pill
Your ST-Link V2 has 10 pins on its output header. The Blue Pill has 4 pins at its bottom edge labeled `GND`, `CLK`, `IO`, `3V3`.

Connect them using DuPont jumper cables as follows:

| Blue Pill Pin Label | ST-Link V2 Pin | Description |
| :---: | :---: | :--- |
| **`3V3`** | **`3.3V`** | 3.3V Power Supply (DO NOT connect to 5V!) |
| **`SWIO` (or `IO`)** | **`SWDIO`** | Serial Wire Debug Data Line |
| **`SWCLK` (or `CLK`)** | **`SWCLK`** | Serial Wire Debug Clock Line |
| **`GND`** | **`GND`** | Common Ground |

> [!WARNING]
> Verify the orientation of the 4-pin header on your Blue Pill. Connecting 3.3V to GND can damage the microcontroller or programmer.

---

### Step 2: Push Button & Resistor on Breadboard
Connect the push button to pin **PA0** with a 10 kΩ pull-down resistor:
1. Place the tactile push button across the center divider of your breadboard.
2. Connect one terminal of the push button to the Blue Pill **3.3V** rail.
3. Connect the other terminal of the button to Blue Pill **PA0**.
4. Connect one leg of the **10 kΩ resistor** to **PA0** (same row as the button output).
5. Connect the other leg of the **10 kΩ resistor** to the **GND** rail.

---

### Step 3: Dual USB Cables Setup
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
5. The onboard green/red LED on the Blue Pill will flash 3 times, signaling ready state.

---

## 5. Live Demonstration & Verification

1. Open a blank text editor like **Notepad** (Windows), **TextEdit** (macOS), or a terminal/browser login box.
2. Place the typing cursor into the editor.
3. **Short Press** the push button on PA0:
   - Status LED turns ON.
   - Microcontroller generates 14-character high-entropy password.
   - Encrypts via AES-128 and stores to internal flash.
   - Types out the password into your editor and automatically presses `ENTER`!
   - LED blinks twice to confirm successful execution.
4. **Long Press** the button (> 2 seconds):
   - The device retrieves and re-types your saved password from flash memory without generating a new one.

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
