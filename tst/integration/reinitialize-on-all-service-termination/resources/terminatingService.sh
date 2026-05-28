#!/usr/bin/env bash

# Brief pause so that UsagiInit's "Init Complete" log lands before our output,
# giving deterministic test snapshots regardless of process scheduling.
sleep 0.1
echo "[TEST SERVICE] Terminating Service Started"
sleep 1
echo "[TEST SERVICE] Terminating Service Exiting"
exit 0
