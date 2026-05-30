#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

cmake . -DCMAKE_BUILD_TYPE=Build \
        -DTERMINATE_ALL_PROCESSES=OFF \
        -DUNIT_TESTING=ON \
        "$@"
cmake --build . --target all
ctest --output-on-failure
