#!/usr/bin/env bash
# Quick launcher for KeyPill ST-Link SWD Companion Daemon
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$SCRIPT_DIR/.venv/bin/python3" "$SCRIPT_DIR/keypill_daemon.py"
