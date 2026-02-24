#!/usr/bin/env bash

# Exit immediately if any command exits with a non-zero status
# This prevents the script from continuing after a failure
set -e

# Create the build directory
# -p ensures no error if the directory already exists
mkdir -p build

# Enter the build directory, run CMake configuration,
# and build the project using the generated Makefile
cd build
cmake ..
make -j4

echo "Build completed successfully"