#!/usr/bin/env bash
# SSH into the VM. Forwards any extra args to ssh.
# Usage:
#   ./ssh_vm.sh                 # interactive shell
#   ./ssh_vm.sh "cd SimpleRouter && ls"
set -eu
source "$(dirname "$0")/../env.sh"

exec ssh $SSH_OPTS "${VM_USER}@${VM_HOST}" "$@"
