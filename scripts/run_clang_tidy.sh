#!/usr/bin/env bash
set -euo pipefail

# Configure the build to export compile commands (required by clang-tidy)
cmake -B build/clang-tidy -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run clang-tidy in parallel using run-clang-tidy on all src and test files
run-clang-tidy -p build/clang-tidy "/(src|test)/"
