#!/usr/bin/env bash

set -euo pipefail

PROJECT_ROOT_NAME="UsagiInit"

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$(cd -P "$(dirname "$SOURCE")" >/dev/null 2>&1 && pwd)"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$(cd -P "$(dirname "$SOURCE")" >/dev/null 2>&1 && pwd)"

CURRENT_DIR="$SCRIPT_DIR"
while [ "$(basename "$CURRENT_DIR")" != "$PROJECT_ROOT_NAME" ]; do
  PARENT_DIR="$(dirname "$CURRENT_DIR")"
  if [ "$PARENT_DIR" = "$CURRENT_DIR" ]; then
    echo "Error: Project root '$PROJECT_ROOT_NAME' not found."
    exit 1
  fi
  CURRENT_DIR="$PARENT_DIR"
done

cd "$CURRENT_DIR" || {
  echo "Failed to change directory to project root: $CURRENT_DIR"
  exit 1
}

cd "tst/integration/env-var-passthrough"
echo "=== RUNNING INTEGRATION TEST CASE: env-var-passthrough ==="

PROGRAM_PATH="../../.."
PROGRAM="$PROGRAM_PATH/UsagiInit"

echo "=== Rebuilding UsagiInit with reinitialize and restart disabled ==="
(
    cd "$PROGRAM_PATH"
    ./scripts/build.sh -DREINITIALIZE_ON_ALL_SERVICE_TERMINATION=OFF \
                       -DRESTART_TERMINATED_SERVICES=OFF \
                       -DRESTART_FAILED_SERVICES=OFF
)
echo "=== Rebuild complete ==="

TMP_OUTPUT=$(mktemp)
TMP_FILTERED_OUTPUT=$(mktemp)
TMP_FILTERED_EXPECTED=$(mktemp)

TEST_ENV_VAR=hello_from_host "$PROGRAM" >"$TMP_OUTPUT" 2>&1 &
PID=$!

echo "=== Test program now running with PID: $PID ==="
sleep 3
kill -SIGINT "$PID"
wait "$PID"

normalize() {
    sed -E \
        -e $'s/\x1B\[[0-9;]*[a-zA-Z]//g' \
        -e 's/\[[A-Z]+\] [0-9]{4}-[0-9]{2}-[0-9]{2} [0-9:]+/[LOGLEVEL] [TIMESTAMP]/g' \
        -e 's/\(.*\/UsagiInit\/src\/[^)]+\)/(SRC_PATH)/g' \
        -e 's/Service added \(PID: [0-9]+\)/Service added (PID: PID)/g' \
        -e 's/Service \(PID: [0-9]+\)/Service (PID: PID)/g' \
        -e 's/fd=[0-9]+/fd=FD/g' \
        -e 's/Service removed \(PID: [0-9]+\)/Service removed (PID: PID)/g' \
        -e 's/ in [0-9]+s\.\.\./\.\.\./g'
}

normalize < "$TMP_OUTPUT" > "$TMP_FILTERED_OUTPUT"
normalize < expected.txt > "$TMP_FILTERED_EXPECTED"

echo "=== Comparing normalized output with expected.txt ==="
if diff -u "$TMP_FILTERED_EXPECTED" "$TMP_FILTERED_OUTPUT"; then
    echo "=== TEST CASE PASSED ==="
    rm -f "$TMP_OUTPUT" "$TMP_FILTERED_OUTPUT" "$TMP_FILTERED_EXPECTED"
else
    echo "=== TEST CASE FAILED: Normalized output differs ==="
    echo "Raw output retained at: $TMP_OUTPUT"
    echo "Normalized output: $TMP_FILTERED_OUTPUT"
    echo "Expected normalized: $TMP_FILTERED_EXPECTED"
    exit 1
fi
