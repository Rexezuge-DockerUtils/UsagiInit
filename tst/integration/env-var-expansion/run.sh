#!/bin/bash

# Set up the test environment
TEST_NAME="env-var-expansion"
TEST_DIR="$(dirname "$0")"
USAGI_INIT_PATH="$(realpath "$TEST_DIR/../../UsagiInit")"

# Rebuild UsagiInit with default flags
cmake -B . -S . -DTERMINATE_ALL_PROCESSES=OFF -DREINITIALIZE_ON_ALL_SERVICE_TERMINATION=OFF > /dev/null 2>&1
cmake --build . > /dev/null 2>&1

# Run the UsagiInit.sh script and capture its output
OUTPUT=$(bash "$TEST_DIR/UsagiInit.sh" 2>&1)

# Compare the output with the expected output
EXPECTED_OUTPUT=$(cat "$TEST_DIR/expected.txt")

if [ "$OUTPUT" == "$EXPECTED_OUTPUT" ]; then
  echo "=== TEST CASE PASSED ==="
else
  echo "=== TEST CASE FAILED ==="
  echo "Expected:"
  echo "$EXPECTED_OUTPUT"
  echo "Got:"
  echo "$OUTPUT"
  exit 1
fi
