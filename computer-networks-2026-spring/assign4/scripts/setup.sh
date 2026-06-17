#!/usr/bin/env bash
# One-time, idempotent VM setup.
#
# Usage: ./vm_setup.sh
set -eu
source "$(dirname "$0")/../env.sh"

have_vm() { VBoxManage list vms | grep -q "\"$VM_NAME\""; }
vm_state() { VBoxManage showvminfo "$VM_NAME" --machinereadable | awk -F= '/^VMState=/{gsub(/"/,"",$2); print $2}'; }

if ! have_vm; then
  echo "[setup] importing $OVA as $VM_NAME..."
  [[ -f "$OVA" ]] || { echo "OVA not found: $OVA"; exit 1; }
  VBoxManage import "$OVA" --vsys 0 --vmname "$VM_NAME"
else
  echo "[setup] VM '$VM_NAME' already registered"
fi

state=$(vm_state || echo unknown)
if [[ "$state" == "running" || "$state" == "paused" ]]; then
  echo "[setup] VM is $state; skipping modifyvm"
else
  echo "[setup] configuring VM (mem=${MEM_MB} cpus=${CPUS})"
  VBoxManage modifyvm "$VM_NAME" --memory "$MEM_MB" --cpus "$CPUS" || true
  VBoxManage modifyvm "$VM_NAME" --natpf1 delete ssh 2>/dev/null || true
  VBoxManage modifyvm "$VM_NAME" --natpf1 "ssh,tcp,,${VM_PORT},,22"
fi

if [[ "$state" != "running" ]]; then
  echo "[setup] starting VM..."
  if [[ "$HEADLESS" == "1" ]]; then
    VBoxManage startvm "$VM_NAME" --type headless
  else
    VBoxManage startvm "$VM_NAME" --type gui
  fi
else
  echo "[setup] VM already running"
fi

echo "[setup] done. ssh with: ./ssh_vm.sh"
