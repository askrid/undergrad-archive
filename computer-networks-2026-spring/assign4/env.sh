#!/usr/bin/env bash
# Shared config sourced by other scripts in this dir.
# Override any var by exporting it before invoking a script:
#   VM_PORT=2222 ./sync.sh up

# Project root (this file's directory)
PROJ_DIR="${PROJ_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"

# VM identity / network
VM_NAME="${VM_NAME:-SimpleRouter_public}"
VM_HOST="${VM_HOST:-127.0.0.1}"
VM_PORT="${VM_PORT:-2224}"
VM_USER="${VM_USER:-student}"

# OVA + VM resources
OVA="${OVA:-$PROJ_DIR/vendor/SimpleRouter_public.ova}"
MEM_MB="${MEM_MB:-4096}"
CPUS="${CPUS:-4}"
HEADLESS="${HEADLESS:-1}"

# Code paths
LOCAL_SRC="${LOCAL_SRC:-$PROJ_DIR/src/}"
REMOTE_SR_DIR="${REMOTE_SR_DIR:-/home/student/SimpleRouter}"
REMOTE_SR_DIR_SLASH="${REMOTE_SR_DIR_SLASH:-$REMOTE_SR_DIR/}"

# Reusable ssh option string
SSH_OPTS="-p $VM_PORT -o StrictHostKeyChecking=accept-new"
