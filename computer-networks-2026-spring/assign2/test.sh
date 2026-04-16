#!/bin/bash

PORT=18090
TOOLDIR="$(dirname "$0")/tools"
TESTDIR="/tmp/shttpd_stress_$$"
PASS=0
FAIL=0
SERVER_PID=

cleanup() {
    [ -n "$SERVER_PID" ] && kill "$SERVER_PID" 2>/dev/null
    wait "$SERVER_PID" 2>/dev/null
    rm -rf "$TESTDIR"
}
trap cleanup EXIT

ok()   { PASS=$((PASS + 1)); echo "  PASS"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL: $1"; }

# ---- setup ----

mkdir -p "$TESTDIR"
dd if=/dev/urandom of="$TESTDIR/file01" bs=1K count=64 2>/dev/null
FILE01_MD5=$(md5sum "$TESTDIR/file01" | cut -d' ' -f1)
dd if=/dev/urandom of="$TESTDIR/bigfile" bs=1M count=100 2>/dev/null
BIG_MD5=$(md5sum "$TESTDIR/bigfile" | cut -d' ' -f1)

cp "$TOOLDIR/fileset" "$TESTDIR/"
(cd "$TESTDIR" && ./fileset -s zipfset -n 100 >/dev/null 2>&1) || true

make -s clean && make -s || { echo "BUILD FAILED"; exit 1; }
echo ""

pkill -f "./shttpd" 2>/dev/null || true
sleep 0.2
./shttpd -p "$PORT" -d "$TESTDIR" &
SERVER_PID=$!
sleep 0.3

echo "==========================================="
echo " shttpd stress test suite"
echo " server PID=$SERVER_PID  port=$PORT"
echo "==========================================="
echo ""

# ---- curl tests (60 pts) ----

echo "[1/11] HTTP/1.0 ephemeral"
OUT=$(curl -s -v --http1.0 -o /dev/null "http://localhost:$PORT/file01" 2>&1)
echo "$OUT" | command grep -q "Connection: close" && echo "$OUT" | command grep -q "shutting down" && ok || fail "expected close + shutdown"

echo "[2/11] HTTP/1.1 persistent"
OUT=$(curl -s -v --http1.1 -o /dev/null "http://localhost:$PORT/file01" 2>&1)
echo "$OUT" | command grep -q "Connection: Keep-Alive" && echo "$OUT" | command grep -q "left intact" && ok || fail "expected Keep-Alive + left intact"

echo "[3/11] HTTP/1.0 + Keep-Alive"
OUT=$(curl -s -v --http1.0 -o /dev/null -H "Connection: Keep-alive" "http://localhost:$PORT/file01" 2>&1)
echo "$OUT" | command grep -q "Connection: Keep-Alive" && echo "$OUT" | command grep -q "left intact" && ok || fail "expected Keep-Alive + left intact"

echo "[4/11] HTTP/1.1 + Close"
OUT=$(curl -s -v --http1.1 -o /dev/null -H "Connection: close" "http://localhost:$PORT/file01" 2>&1)
echo "$OUT" | command grep -q "Connection: close" && echo "$OUT" | command grep -q "shutting down" && ok || fail "expected close + shutdown"

echo "[5/11] 5 concurrent HTTP/1.0 ephemeral"
PIDS=""
TMPF="$TESTDIR/concurrent_out"
: > "$TMPF"
for i in $(seq 1 5); do
    curl -s -o /dev/null -w "%{http_code}\n" --http1.0 "http://localhost:$PORT/file01" >> "$TMPF" &
    PIDS="$PIDS $!"
done
for p in $PIDS; do wait "$p" 2>/dev/null; done
CODES=$(sort "$TMPF" | tr -d '\n')
[ "$CODES" = "200200200200200" ] && ok || fail "got $CODES"

echo "[6/11] 404 error"
CODE=$(curl -s -o /dev/null -w "%{http_code}" --http1.0 "http://localhost:$PORT/nonexistent")
[ "$CODE" = "404" ] && ok || fail "expected 404, got $CODE"

echo "[7/11] 400 error (no Host header)"
RESP=$(echo -ne "GET /file01 HTTP/1.0\r\n\r\n" | bash -c "exec 3<>/dev/tcp/localhost/$PORT; cat >&3; head -1 <&3" 2>/dev/null)
echo "$RESP" | command grep -q "400 Bad Request" && ok || fail "expected 400, got: $RESP"

# ---- integrity tests ----

echo "[8/11] 100MB file integrity (HTTP/1.0)"
GOT=$(curl -s --http1.0 "http://localhost:$PORT/bigfile" | md5sum | cut -d' ' -f1)
[ "$GOT" = "$BIG_MD5" ] && ok || fail "md5 mismatch: $GOT != $BIG_MD5"

echo "[9/11] 100MB file integrity (HTTP/1.1)"
GOT=$(curl -s --http1.1 "http://localhost:$PORT/bigfile" | md5sum | cut -d' ' -f1)
[ "$GOT" = "$BIG_MD5" ] && ok || fail "md5 mismatch: $GOT != $BIG_MD5"

# ---- flexiclient tests (20 pts) ----

echo "[10/11] flexiclient single, 5 persistent conns, 10s"
OUT=$("$TOOLDIR/single" /file01 | "$TOOLDIR/flexiclient" \
    -host localhost -port "$PORT" -active 5 -time 10 \
    -exhdrs "Host: localhost\r\n" -persist force 2>&1)
REQS=$(echo "$OUT" | command grep '^#\[' | awk '{print $6}')
if [ -n "$REQS" ] && [ "$REQS" -gt 0 ] 2>/dev/null; then
    echo "  ($REQS reqs served)"
    ok
else
    fail "flexiclient reported 0 or no requests"
fi

echo "[11/11] flexiclient zipfian, 5 persistent conns, 10s"
OUT=$("$TOOLDIR/zipfgen" -s spec -n 100 | "$TOOLDIR/flexiclient" \
    -host localhost -port "$PORT" -active 5 -time 10 \
    -exhdrs "Host: localhost\r\n" 2>&1)
REQS=$(echo "$OUT" | command grep '^#\[' | awk '{print $6}')
if [ -n "$REQS" ] && [ "$REQS" -gt 0 ] 2>/dev/null; then
    echo "  ($REQS reqs served)"
    ok
else
    fail "flexiclient reported 0 or no requests"
fi

# ---- summary ----

echo ""
echo "==========================================="
echo " Results: $PASS passed, $FAIL failed"
echo "==========================================="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
