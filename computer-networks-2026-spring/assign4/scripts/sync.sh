#!/usr/bin/env bash
# rsync host src/ <-> VM ~/SimpleRouter/
# Usage:
#   ./sync.sh up      # host -> VM
#   ./sync.sh down    # VM   -> host
#   ./sync.sh         # default: up
set -eu
source "$(dirname "$0")/../env.sh"

DIR="${1:-up}"

RSYNC_OPTS=(
  -avz
  --exclude '.run_logs/'
  --exclude '*.o'
  --exclude '.*.d'
  --exclude '/router/sr'
  -e "ssh $SSH_OPTS"
)

case "$DIR" in
  up)
    rsync "${RSYNC_OPTS[@]}" "$LOCAL_SRC" "${VM_USER}@${VM_HOST}:${REMOTE_SR_DIR_SLASH}"
    ;;
  down)
    # Only pull back router/ + run_all.sh, not all of ~/SimpleRouter/
    rsync "${RSYNC_OPTS[@]}" \
      "${VM_USER}@${VM_HOST}:${REMOTE_SR_DIR}/router/" \
      "$LOCAL_SRC/router/"
    rsync "${RSYNC_OPTS[@]}" \
      "${VM_USER}@${VM_HOST}:${REMOTE_SR_DIR}/run_all.sh" \
      "$LOCAL_SRC/run_all.sh" || true
    ;;
  *)
    echo "usage: $0 [up|down]" >&2
    exit 1
    ;;
esac
