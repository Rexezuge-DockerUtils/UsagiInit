#!/usr/bin/env bash

set -euo pipefail

TEST_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
PROJECT_ROOT="$(cd "$TEST_DIR/../../.." && pwd)"
PROGRAM="$PROJECT_ROOT/UsagiInit"

echo "=== RUNNING INTEGRATION TEST CASE: stderr-redirection ==="

(
  cd "$PROJECT_ROOT"
  ./scripts/build.sh -DTERMINATE_ALL_PROCESSES=OFF \
    -DREINITIALIZE_ON_ALL_SERVICE_TERMINATION=OFF > /dev/null
)

TMP_OUTPUT="$(mktemp)"
TMP_FILTERED_OUTPUT="$(mktemp)"

cleanup() {
  rm -f "$TMP_OUTPUT" "$TMP_FILTERED_OUTPUT" "$TEST_DIR/stderr-output.txt"
}
trap cleanup EXIT

rm -f "$TEST_DIR/stderr-output.txt"

(
  cd "$TEST_DIR"
  "$PROGRAM" ./UsagiInit.sh > "$TMP_OUTPUT" 2>&1
)

sed -n '/^redirect-/p' "$TMP_OUTPUT" > "$TMP_FILTERED_OUTPUT"

echo "=== Comparing filtered output with expected.txt ==="
if diff -u "$TEST_DIR/expected.txt" "$TMP_FILTERED_OUTPUT"; then
  echo "=== TEST CASE PASSED ==="
else
  echo "=== TEST CASE FAILED: Filtered output differs ==="
  echo "Raw output retained at: $TMP_OUTPUT"
  trap - EXIT
  exit 1
fi
