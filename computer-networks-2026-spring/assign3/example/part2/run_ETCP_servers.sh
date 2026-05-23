#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   sudo ./run_etcp_servers.sh [NUM_SERVERS] [START_IP_LAST_OCTET] [START_PORT]
#
# Example:
#   sudo ./run_etcp_servers.sh 10 13 10000
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
WWW_HOME="www_home"

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

if [[ ! -d "$WWW_HOME" ]]; then
    echo "Error: $WWW_HOME directory not found."
    exit 1
fi

echo "Starting $NUM_SERVERS eTCP servers..."
echo "Interface: $IFACE"
echo "IP start:  ${IP_PREFIX}.${START_IP_LAST_OCTET}"
echo "Port start: $START_PORT"
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
tcp_timewait = 5

# fault injection code
drop_data_rate = 0
drop_ack_rate = 0
drop_data_nth = 0
drop_incoming_ack_nth = 0
drop_incoming_ack_count = 0
loss_seed = 40
EOF

    echo "[START] ${ip_addr}:${port}"
    echo "        config: $conf_file"
    echo "        log:    $log_file"

    sudo "$SERVER_BIN" "$port" -f "$conf_file" -p "$WWW_HOME" > "$log_file" 2>&1 &
    PIDS+=("$!")
done

echo
echo "All servers started."
echo "PIDs: ${PIDS[*]}"
echo
echo "Press Ctrl+C to stop all servers."

wait
