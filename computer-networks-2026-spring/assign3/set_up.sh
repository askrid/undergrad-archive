#!/usr/bin/env bash
set -euo pipefail

############################################################
# Student setup
# if your Machine is nw01 -> type 1
# if your Machine is nw02 -> type 2
# if your Machine is nw03 -> type 3
# if your Machine is nw04 -> type 4
############################################################
MACHINE_ID=3
############################################################

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PART1_DIR="$ROOT_DIR/example/part1"
PART2_DIR="$ROOT_DIR/example/part2"
CFG1_DIR="$PART1_DIR/config"
CFG2_DIR="$PART2_DIR/config"

mkdir -p "$CFG1_DIR" "$CFG2_DIR"

case "$MACHINE_ID" in
  1)
    ARP_CONTENT=$'ARP_ENTRY 1\n10.70.0.12/16 bc:24:11:7a:9a:52'
    ;;
  3)
    ARP_CONTENT=$'ARP_ENTRY 1\n192.168.0.11/24 bc:24:11:07:ef:2c'
    ;;
  2)
    ARP_CONTENT=$'ARP_ENTRY 1\n10.70.0.11/16 bc:24:11:39:06:eb'
    ;;
  4)
    ARP_CONTENT=$'ARP_ENTRY 1\n192.168.0.10/24 bc:24:11:0b:9b:b4'
    ;;
  *)
    echo "Invalid MACHINE_ID: $MACHINE_ID (must be 1, 2, 3, or 4)" >&2
    exit 1
    ;;
esac

printf '%s\n' "$ARP_CONTENT" > "$CFG1_DIR/arp.conf"
printf '%s\n' "$ARP_CONTENT" > "$CFG2_DIR/arp.conf"

echo "Created:"
echo "  $CFG1_DIR/arp.conf"
echo "  $CFG2_DIR/arp.conf"
