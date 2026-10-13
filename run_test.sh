#!/usr/bin/env bash
set -e

if [ -z "$1" ]; then
    echo "bash run_test.sh"
    exit 1
fi
# Strip .c extension if it was passed (e.g. test_models.c -> test_models)
TEST_NAME="${1%.c}"

# Compile all necessary source files
gcc -I./src "src/tests/${TEST_NAME}.c" src/models.c src/common.c -o "$TEST_NAME"

TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT

# Run only if compilation succeeded
TEST_BINARY="$(pwd)/$TEST_NAME"

(
    cd "$TEST_DIR"
    "$TEST_BINARY"
)