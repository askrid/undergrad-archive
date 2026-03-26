#!/bin/bash
set -e

PORT=${1:-12345}
NUM_CLIENTS=${2:-5}
SERVER=${3:-localhost}

LOGDIR=$(mktemp -d)
trap 'kill $SERVER_PID 2>/dev/null; wait 2>/dev/null; rm -rf "$LOGDIR"' EXIT

# start server
./sserver -p "$PORT" 2>"$LOGDIR/server.log" &
SERVER_PID=$!
sleep 0.3

# launch concurrent clients
for i in $(seq 1 "$NUM_CLIENTS"); do
    echo "message from client $i" \
        | ./sclient -p "$PORT" -s "$SERVER" \
        >"$LOGDIR/client${i}.out" 2>"$LOGDIR/client${i}.log" &
done
wait $(jobs -rp | grep -v "$SERVER_PID")

# report results
PASS=0
FAIL=0
for i in $(seq 1 "$NUM_CLIENTS"); do
    EXPECTED="message from client $i"
    ACTUAL=$(cat "$LOGDIR/client${i}.out")
    if [ "$ACTUAL" = "$EXPECTED" ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        echo "FAIL client $i: expected '$EXPECTED', got '$ACTUAL'"
    fi
done

echo "$PASS/$NUM_CLIENTS passed"
if [ "$FAIL" -gt 0 ]; then
    echo "--- server log ---"
    cat "$LOGDIR/server.log"
    exit 1
fi
