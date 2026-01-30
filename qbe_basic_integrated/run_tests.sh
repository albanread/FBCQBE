#!/bin/bash

# Simple test runner for FasterBASIC QBE compiler
# Compiles and runs test files, reports pass/fail

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
COMPILER="$PROJECT_ROOT/qbe_basic"
TEST_DIR="$PROJECT_ROOT/../fsh"
RUNTIME_C="$TEST_DIR/FasterBASICT/runtime_c"
TEMP_DIR="/tmp/qbe_basic_tests"

# Create temp directory
mkdir -p "$TEMP_DIR"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counters
TOTAL=0
PASSED=0
FAILED=0
COMPILE_FAILED=0

echo "========================================"
echo "FasterBASIC QBE Test Runner"
echo "========================================"
echo ""

# Find all test files
TEST_FILES=$(find "$TEST_DIR" -maxdepth 1 -name "test*.bas" | sort)

for TEST_FILE in $TEST_FILES; do
    TEST_NAME=$(basename "$TEST_FILE" .bas)
    TOTAL=$((TOTAL + 1))

    printf "%-40s" "$TEST_NAME..."

    # Compile to assembly
    if ! "$COMPILER" "$TEST_FILE" > "$TEMP_DIR/$TEST_NAME.s" 2>&1; then
        echo -e "${RED}COMPILE FAIL${NC}"
        COMPILE_FAILED=$((COMPILE_FAILED + 1))
        FAILED=$((FAILED + 1))
        continue
    fi

    # Link with runtime
    if ! gcc "$TEMP_DIR/$TEST_NAME.s" "$RUNTIME_C"/*.c -I"$RUNTIME_C" -lm -o "$TEMP_DIR/$TEST_NAME" 2>&1 | grep -v "warning:"; then
        echo -e "${RED}LINK FAIL${NC}"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Run the test (with timeout)
    if timeout 5s "$TEMP_DIR/$TEST_NAME" > "$TEMP_DIR/$TEST_NAME.out" 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        EXIT_CODE=$?
        if [ $EXIT_CODE -eq 124 ]; then
            echo -e "${YELLOW}TIMEOUT${NC}"
        else
            echo -e "${RED}FAIL (exit $EXIT_CODE)${NC}"
        fi
        FAILED=$((FAILED + 1))
    fi
done

echo ""
echo "========================================"
echo "Test Results"
echo "========================================"
echo "Total:          $TOTAL"
echo -e "Passed:         ${GREEN}$PASSED${NC}"
echo -e "Failed:         ${RED}$FAILED${NC}"
echo -e "Compile Failed: ${RED}$COMPILE_FAILED${NC}"
echo "========================================"

# Clean up
# rm -rf "$TEMP_DIR"

exit $FAILED
