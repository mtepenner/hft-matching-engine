#!/usr/bin/env bash
# core_pinning.sh — Helper to pin the exchange_core process to isolated CPUs.
# Usage: ./scripts/core_pinning.sh <pid> [core_list]
# Example: ./scripts/core_pinning.sh 12345 2,3
set -euo pipefail

PID="${1:-}"
CORES="${2:-2}"

if [[ -z "$PID" ]]; then
  echo "Usage: $0 <pid> [core_list]"
  echo "  core_list: comma-separated CPU IDs, e.g. 2,3"
  exit 1
fi

if ! command -v taskset &>/dev/null; then
  echo "[ERROR] taskset not found. Install util-linux."
  exit 1
fi

echo "[CorePin] Pinning PID $PID to core(s): $CORES"
taskset -cp "$CORES" "$PID"

echo "[CorePin] Verifying affinity:"
taskset -cp "$PID"

# Optionally set SCHED_FIFO real-time priority (requires root)
if [[ "${SET_RT:-0}" == "1" ]]; then
  echo "[CorePin] Setting SCHED_FIFO priority 99 (requires root)"
  chrt -f -p 99 "$PID"
fi

echo "[CorePin] Done."
