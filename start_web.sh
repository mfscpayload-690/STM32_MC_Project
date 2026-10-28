#!/usr/bin/env bash
# ==============================================================================
# KeyPill Hardware Portal Launcher
# ==============================================================================

PORT=8080
WEB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/web" && pwd)"

echo "=================================================="
echo "  KeyPill Hardware Testing Web Portal"
echo "  Target: STM32 Blue Pill (STM32F103C8T6)"
echo "=================================================="
echo "[*] Serving from: ${WEB_DIR}"
echo "[*] URL: http://localhost:${PORT}"
echo ""

# Try opening in browser
if command -v xdg-open >/dev/null 2>&1; then
    (sleep 1 && xdg-open "http://localhost:${PORT}" >/dev/null 2>&1) &
elif command -v sensible-browser >/dev/null 2>&1; then
    (sleep 1 && sensible-browser "http://localhost:${PORT}" >/dev/null 2>&1) &
fi

echo "[*] Starting local HTTP server on port ${PORT}... (Press Ctrl+C to stop)"
python3 -m http.server ${PORT} --directory "${WEB_DIR}"
