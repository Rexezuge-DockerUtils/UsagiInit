#!/bin/bash

# Exit on any error
set -e

echo "=== BUILDING ==="

# Configure the project
cmake . -DCMAKE_BUILD_TYPE=Build -DCMAKE_FIND_LIBRARY_SUFFIXES=".a"

# Build the project
cmake --build .

echo "=== BUILD COMPLETED SUCCESSFULLY ==="
