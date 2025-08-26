#!/usr/bin/env bash

set -euo pipefail

## >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
## ===== Locate Project Root =====

# Name of your project root directory
PROJECT_ROOT_NAME="UsagiInit"

# Resolve the full path of the script, following symlinks
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$(cd -P "$(dirname "$SOURCE")" >/dev/null 2>&1 && pwd)"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
SCRIPT_DIR="$(cd -P "$(dirname "$SOURCE")" >/dev/null 2>&1 && pwd)"

# Traverse up to find the project root
CURRENT_DIR="$SCRIPT_DIR"
while [ "$(basename "$CURRENT_DIR")" != "$PROJECT_ROOT_NAME" ]; do
  PARENT_DIR="$(dirname "$CURRENT_DIR")"
  if [ "$PARENT_DIR" = "$CURRENT_DIR" ]; then
    echo "Error: Project root '$PROJECT_ROOT_NAME' not found."
    exit 1
  fi
  CURRENT_DIR="$PARENT_DIR"
done

# Change to the project root directory
cd "$CURRENT_DIR" || {
  echo "Failed to change directory to project root: $CURRENT_DIR"
  exit 1
}

## ===== Locate Project Root =====
## <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

## >>>>>>>>>>>>>>>>>>>>>>>>>>>>>
## ===== INTEGRATION TESTS =====

find "tst/integration" -mindepth 2 -name "run.sh" -exec sh -c 'cd "$(dirname "{}")" && bash "$(basename "{}")"' \;

## ===== INTEGRATION TESTS =====
## <<<<<<<<<<<<<<<<<<<<<<<<<<<<<
