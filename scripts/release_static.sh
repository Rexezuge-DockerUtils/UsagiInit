#!/bin/bash

# Exit on any error
set -e

echo "=== BUILDING (RELEASE-STATIC) ==="

# Configure the project
cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-static" -DCMAKE_FIND_LIBRARY_SUFFIXES=".a"

# Build the project
cmake --build .

echo "=== RELEASE-STATIC BUILD COMPLETED SUCCESSFULLY ==="
