#!/bin/sh
# Usage: source activate.sh [PORT]

PORT="${1:-/dev/cu.usbmodem1422301}"
VENV=".venv"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"

if [ ! -e "$PORT" ]; then
    echo "Port not found: $PORT"
    echo "Available ports:"
    ls /dev/cu.* 2>/dev/null || echo "  (none)"
    return 1 2>/dev/null || exit 1
fi

if [ ! -d "$VENV" ]; then
    echo "Creating venv..."
    python3 -m venv "$VENV"
    "$VENV/bin/pip" install --upgrade pip
    "$VENV/bin/pip" install -r requirements.txt
fi

source "$VENV/bin/activate"
alias mp="mpremote connect $PORT"

sync() {
    echo "Syncing board/ to ESP32..."
    mpremote connect "$PORT" mkdir :core 2>/dev/null; true
    mpremote connect "$PORT" cp "$SCRIPT_DIR"/board/main.py :main.py
    mpremote connect "$PORT" cp "$SCRIPT_DIR"/board/core/*.py :core/
    echo "Done!"
    mpremote connect "$PORT" ls
}

echo "Ready! (port: $PORT)"
echo "  mp run board/main.py  - run a script"
echo "  mp repl               - open REPL"
echo "  mp ls                 - list files on board"
echo "  sync                  - upload board/ to ESP32"
