#!/usr/bin/env python3
"""
==============================================================================
Project: KeyPill - ST-Link SWD Companion Daemon
==============================================================================
Description:
    Bypasses Micro-USB hardware issues by reading passwords directly from the
    STM32 Blue Pill's secure SRAM Mailbox via ST-Link SWD in real-time.
    When an Auto-Fill request is initiated on KeyPill, this daemon reads the
    decrypted password and types it into the active/focused window using xdotool.

Requirements:
    - python3 with pyusb (available in project's .venv)
    - xdotool (`/usr/bin/xdotool`)
    - ST-Link V2 plugged into USB
==============================================================================
"""

import sys
import os
import time
import subprocess
import struct

# Add bundled pystlink library to Python path
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PYSTLINK_DIR = os.path.join(SCRIPT_DIR, "tools", "pystlink")
if PYSTLINK_DIR not in sys.path:
    sys.path.insert(0, PYSTLINK_DIR)

try:
    import lib.dbg
    import lib.stlinkusb
    import lib.stlinkv2
except ImportError as err:
    print(f"[Error] Failed to import bundled pystlink: {err}")
    print("Ensure tools/pystlink/lib is present.")
    sys.exit(1)

# Mailbox Protocol Constants (Matches KeyPill.ino)
MAILBOX_MAGIC        = 0x4B50494C  # 'KPIL' (little-endian)
MAILBOX_IDLE         = 0
MAILBOX_PENDING_TYPE = 1
MAILBOX_TYPED_OK     = 2
MAILBOX_ERROR        = 3

def locate_mailbox_symbol():
    """Attempts to find g_mailbox address from KeyPill/build or Arduino cache .elf file."""
    search_dirs = [
        os.path.join(SCRIPT_DIR, "KeyPill", "build"),
        os.path.expanduser("~/.cache/arduino/sketches"),
    ]
    for root_dir in search_dirs:
        if not os.path.exists(root_dir):
            continue
        for root, _, files in os.walk(root_dir):
            for file in files:
                if file.endswith("KeyPill.ino.elf"):
                    elf_path = os.path.join(root, file)
                    try:
                        out = subprocess.check_output(
                            ["nm", elf_path], stderr=subprocess.DEVNULL, text=True
                        )
                        for line in out.splitlines():
                            if " g_mailbox" in line:
                                addr_str = line.split()[0]
                                addr = int(addr_str, 16)
                                return addr
                    except Exception:
                        pass
    return None

def scan_mailbox_in_sram(stlink):
    """Scans STM32F103 SRAM (0x20000000 to 0x20004000) for the 'KPIL' magic word."""
    sram_start = 0x20000000
    sram_end   = 0x20004000  # Scan first 16KB of SRAM
    step       = 256

    for addr in range(sram_start, sram_end, step):
        try:
            raw_bytes = bytes(stlink.get_mem32(addr, step))
            words = struct.unpack(f"<{step // 4}I", raw_bytes)
            for i, val in enumerate(words):
                if val == MAILBOX_MAGIC:
                    found_addr = addr + (i * 4)
                    return found_addr
        except Exception:
            break
    return None

def connect_stlink():
    """Connects to ST-Link V2 and enters SWD debug mode."""
    dbg = lib.dbg.Dbg(verbose=0)
    try:
        connector = lib.stlinkusb.StlinkUsbConnector(dbg=dbg)
        stlink = lib.stlinkv2.Stlink(connector, dbg=dbg)
        stlink.enter_debug_swd()
        return stlink
    except Exception as e:
        return None

def type_password(password):
    """Types password into currently focused window using xdotool."""
    if not password:
        return False
    try:
        # --clearmodifiers ensures caps/ctrl/alt are not held
        # --delay 15 sets 15ms per character for rock-solid input acceptance
        subprocess.run(
            ["xdotool", "type", "--clearmodifiers", "--delay", "15", "--", password],
            check=True
        )
        return True
    except subprocess.CalledProcessError as e:
        print(f"[Error] xdotool failed: {e}")
        return False
    except FileNotFoundError:
        print("[Error] xdotool not found! Install it with: sudo pacman -S xdotool")
        return False

def main():
    print("==================================================================")
    print("  KeyPill ST-Link SWD Companion Daemon")
    print("  Direct Hardware Mailbox -> Linux Keystroke Auto-Filler")
    print("==================================================================")

    mailbox_addr = locate_mailbox_symbol()
    if mailbox_addr:
        print(f"[*] Found g_mailbox in ELF symbol table: 0x{mailbox_addr:08X}")
    else:
        print("[*] Mailbox not found in ELF, will dynamically scan STM32 SRAM.")

    stlink = None
    while True:
        try:
            if stlink is None:
                stlink = connect_stlink()
                if stlink is None:
                    print("[Waiting] ST-Link V2 not found. Plug ST-Link into PC USB...", end="\r", flush=True)
                    time.sleep(1.0)
                    continue

                print("\n[Connected] ST-Link V2 connected via SWD!")
                if mailbox_addr is None:
                    print("[*] Scanning STM32 SRAM for KeyPill mailbox...")
                    mailbox_addr = scan_mailbox_in_sram(stlink)
                    if mailbox_addr is None:
                        print("[Notice] Mailbox 'KPIL' not yet found in SRAM. Is KeyPill firmware running?")
                        time.sleep(1.0)
                        continue
                    print(f"[Ready] KeyPill Mailbox detected at 0x{mailbox_addr:08X}!")
                print("[Listening] Ready for Auto-Fill requests from KeyPill knob...")

            # Poll mailbox header: 12 bytes = [magic (4B), status (4B), length (4B)]
            header_bytes = bytes(stlink.get_mem32(mailbox_addr, 12))
            magic, status, length = struct.unpack("<III", header_bytes)

            if magic != MAILBOX_MAGIC:
                # Target reloaded / memory shifted: re-scan
                print("[*] Mailbox magic changed, re-scanning SRAM...")
                mailbox_addr = scan_mailbox_in_sram(stlink)
                time.sleep(0.5)
                continue

            if status == MAILBOX_PENDING_TYPE:
                if 0 < length <= 64:
                    # Read password buffer (rounded up to 4 bytes for get_mem32)
                    read_len = (length + 3) & ~3
                    pwd_raw = bytes(stlink.get_mem32(mailbox_addr + 12, read_len))[:length]
                    try:
                        pwd_str = pwd_raw.decode("ascii", errors="ignore")
                    except Exception:
                        pwd_str = ""

                    if pwd_str:
                        # Type password into active window
                        type_ok = type_password(pwd_str)
                        # Acknowledge to KeyPill (status at offset +4)
                        ack_val = MAILBOX_TYPED_OK if type_ok else MAILBOX_ERROR
                        stlink.set_mem32(mailbox_addr + 4, list(struct.pack("<I", ack_val)))
                        print(f"[{time.strftime('%H:%M:%S')}] Auto-filled password ({length} chars) into focused window.")
                        del pwd_str
                        del pwd_raw

            time.sleep(0.04)  # 25Hz polling rate: <40ms response time, ~0.02% CPU usage

        except KeyboardInterrupt:
            print("\n[Exiting] KeyPill companion daemon stopped by user.")
            break
        except Exception as e:
            # Handle ST-Link disconnects gracefully
            stlink = None
            time.sleep(1.0)

if __name__ == "__main__":
    main()
