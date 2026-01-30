#!/bin/bash
# UDT Test Runner Script
# Compiles and runs UDT tests using qbe_basic

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
QBE_BASIC="$SCRIPT_DIR/qbe_basic"
TEST_DIR="$SCRIPT_DIR/../tests/types"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

run_test() {
    local test_name="$1"
    local test_file="$TEST_DIR/$test_name.bas"

    if [ ! -f "$test_file" ]; then
        echo -e "${RED}✗${NC} Test file not found: $test_file"
        return 1
    fi

    echo -e "${YELLOW}Running:${NC} $test_name"

    # Compile to QBE IL
    if ! "$QBE_BASIC" -i "$test_file" > "/tmp/$test_name.ssa" 2>&1; then
        echo -e "${RED}✗ FAIL${NC} - Compilation failed"
        cat "/tmp/$test_name.ssa"
        return 1
    fi

    # Generate assembly with QBE
    if ! "$SCRIPT_DIR/../fsh/package/qbe/qbe" "/tmp/$test_name.ssa" > "/tmp/$test_name.s" 2>&1; then
        echo -e "${RED}✗ FAIL${NC} - QBE codegen failed"
        cat "/tmp/$test_name.s"
        return 1
    fi

    # Compile runtime if needed
    if [ ! -d "$SCRIPT_DIR/runtime/.obj" ]; then
        echo "  Building runtime..."
        mkdir -p "$SCRIPT_DIR/runtime/.obj"
        for c_file in "$SCRIPT_DIR/runtime"/*.c; do
            base=$(basename "$c_file" .c)
            clang -c -O2 "$c_file" -o "$SCRIPT_DIR/runtime/.obj/${base}.o" 2>&1
        done
    fi

    # Link executable
    if ! clang "/tmp/$test_name.s" "$SCRIPT_DIR/runtime/.obj"/*.o -o "/tmp/$test_name" 2>&1; then
        echo -e "${RED}✗ FAIL${NC} - Linking failed"
        return 1
    fi

    # Run test
    echo -e "  ${YELLOW}Output:${NC}"
    if "/tmp/$test_name"; then
        echo -e "${GREEN}✓ PASS${NC}"
        return 0
    else
        echo -e "${RED}✗ FAIL${NC} - Runtime error"
        return 1
    fi
}

# Run tests
echo "========================================"
echo "UDT Test Suite"
echo "========================================"
echo ""

passed=0
failed=0

tests=(
    "test_udt_simple"
    "test_udt_twofields"
    "test_udt_nested"
    "test_udt_string"
    "test_udt_array"
    "test_udt_long"
)

for test in "${tests[@]}"; do
    if run_test "$test"; then
        ((passed++))
    else
        ((failed++))
    fi
    echo ""
done

echo "========================================"
echo "Results: $passed passed, $failed failed"
echo "========================================"

if [ $failed -eq 0 ]; then
    exit 0
else
    exit 1
fi
