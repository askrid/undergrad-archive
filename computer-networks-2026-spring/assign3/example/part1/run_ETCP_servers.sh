#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   sudo ./run_etcp_servers.sh [NUM_SERVERS] [START_IP_LAST_OCTET] [START_PORT]
#
# Example:
#   sudo ./run_etcp_servers.sh 30 13 10000
#
# This runs:
#   192.168.0.13 : 10000
#   192.168.0.15 : 10020
#   192.168.0.17 : 10040
#   ...

NUM_SERVERS="${1:-30}"
START_IP_LAST_OCTET="${2:-13}"
START_PORT="${3:-10000}"

IFACE="ens19"
IP_PREFIX="192.168.0"
IP_STEP=2
PORT_STEP=20

SERVER_BIN="./ETCP_server"
CONF_DIR="./generated_confs"
LOG_DIR="./server_logs"

mkdir -p "$CONF_DIR" "$LOG_DIR"

if [[ $EUID -ne 0 ]]; then
    echo "Please run with sudo."
    exit 1
fi

if [[ ! -x "$SERVER_BIN" ]]; then
    echo "Error: $SERVER_BIN not found or not executable."
    exit 1
fi

echo "Starting $NUM_SERVERS eTCP servers..."
echo "Interface:  $IFACE"
echo "Start IP:   ${IP_PREFIX}.${START_IP_LAST_OCTET}"
echo "Start Port: $START_PORT"
echo

PIDS=()

cleanup() {
    echo
    echo "Stopping eTCP servers..."
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done
    wait || true
    echo "Done."
}

trap cleanup INT TERM

for ((i=0; i<NUM_SERVERS; i++)); do
    ip_last=$((START_IP_LAST_OCTET + i * IP_STEP))
    ip_addr="${IP_PREFIX}.${ip_last}"
    port=$((START_PORT + i * PORT_STEP))

    conf_file="${CONF_DIR}/ETCP_${ip_addr}_${port}.conf"
    log_file="${LOG_DIR}/server_${ip_addr}_${port}.log"

    cat > "$conf_file" <<EOF
io = pcap
num_cores = 1
port = ${IFACE}
ip = ${ip_addr}
rcvbuf = 65536
sndbuf = 65536
tcp_timeout = 60
tcp_timewait = 2

# fault injection code (connection management)
drop_syn_nth = 5
drop_synack_nth = 3
drop_fin_nth = 2
EOF

    echo "[START] ${ip_addr}:${port}"
    echo "        config: $conf_file"
    echo "        log:    $log_file"

    "$SERVER_BIN" "$port" -f "$conf_file" > "$log_file" 2>&1 &
    PIDS+=("$!")
done

echo
echo "All servers started."
echo "PIDs: ${PIDS[*]}"
echo
echo "Press Ctrl+C to stop all servers."

wait
