#!/usr/bin/env bash

# Exit immediately if any command fails
set -e

# Check whether the "build" directory exists
if [ -d "build" ]; then
  # Remove the build directory and all its contents
  rm -rf build
  echo "Build directory removed"
else
  # Inform the user if the build directory does not exist
  echo "Build directory does not exist"
fi