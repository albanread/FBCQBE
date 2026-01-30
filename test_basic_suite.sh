#!/bin/bash
#
# test_basic_suite.sh
# Comprehensive test runner for FasterBASIC compiler
#
# This script tests the qbe_basic compiler with various BASIC programs
# to ensure type system changes work correctly with QBE backend
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QBE_BASIC="$SCRIPT_DIR/qbe_basic_integrated/qbe_basic"
RUNTIME_DIR="$SCRIPT_DIR/fsh/FasterBASICT/runtime_c"
TEST_DIR="$SCRIPT_DIR/fsh"
TEMP_DIR="$SCRIPT_DIR/test_output"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Create temp directory for test outputs
mkdir -p "$TEMP_DIR"

# Log file
LOG_FILE="$TEMP_DIR/test_results.log"
echo "=== FasterBASIC Test Suite ===" > "$LOG_FILE"
echo "Date: $(date)" >> "$LOG_FILE"
echo "" >> "$LOG_FILE"

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "PASS")
            echo -e "${GREEN}✓ PASS${NC}: $message"
            echo "✓ PASS: $message" >> "$LOG_FILE"
            ;;
        "FAIL")
            echo -e "${RED}✗ FAIL${NC}: $message"
            echo "✗ FAIL: $message" >> "$LOG_FILE"
            ;;
        "SKIP")
            echo -e "${YELLOW}○ SKIP${NC}: $message"
            echo "○ SKIP: $message" >> "$LOG_FILE"
            ;;
        "INFO")
            echo -e "${BLUE}ℹ INFO${NC}: $message"
            echo "ℹ INFO: $message" >> "$LOG_FILE"
            ;;
    esac
}

# Function to test a single BASIC file
test_basic_file() {
    local bas_file=$1
    local test_name=$(basename "$bas_file" .bas)
    local expected_failure=${2:-0}

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    print_status "INFO" "Testing: $test_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # Step 1: Compile BASIC to QBE IL
    local qbe_file="$TEMP_DIR/${test_name}.qbe"
    local asm_file="$TEMP_DIR/${test_name}.s"
    local exe_file="$TEMP_DIR/${test_name}"

    echo "  [1/4] Compiling BASIC to QBE IL..."
    if ! "$QBE_BASIC" -i -o "$qbe_file" "$bas_file" 2>"$TEMP_DIR/${test_name}.compile.err"; then
        if [ $expected_failure -eq 1 ]; then
            print_status "PASS" "$test_name (expected compilation failure)"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            cat "$TEMP_DIR/${test_name}.compile.err" >> "$LOG_FILE"
            return 0
        else
            print_status "FAIL" "$test_name - Compilation to QBE IL failed"
            cat "$TEMP_DIR/${test_name}.compile.err"
            cat "$TEMP_DIR/${test_name}.compile.err" >> "$LOG_FILE"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    fi

    echo "  [2/4] Compiling QBE IL to Assembly..."
    if ! "$QBE_BASIC" -o "$asm_file" "$qbe_file" 2>"$TEMP_DIR/${test_name}.qbe.err"; then
        print_status "FAIL" "$test_name - QBE IL to Assembly failed"
        cat "$TEMP_DIR/${test_name}.qbe.err"
        cat "$TEMP_DIR/${test_name}.qbe.err" >> "$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi

    echo "  [3/4] Linking with runtime..."
    if ! cc "$asm_file" \
        "$RUNTIME_DIR/array_descriptor_runtime.c" \
        "$RUNTIME_DIR/array_ops.c" \
        "$RUNTIME_DIR/basic_data.c" \
        "$RUNTIME_DIR/basic_runtime.c" \
        "$RUNTIME_DIR/conversion_ops.c" \
        "$RUNTIME_DIR/io_ops.c" \
        "$RUNTIME_DIR/io_ops_format.c" \
        "$RUNTIME_DIR/math_ops.c" \
        "$RUNTIME_DIR/memory_mgmt.c" \
        "$RUNTIME_DIR/string_ops.c" \
        "$RUNTIME_DIR/string_pool.c" \
        "$RUNTIME_DIR/string_utf32.c" \
        -I"$RUNTIME_DIR" \
        -lm \
        -o "$exe_file" 2>"$TEMP_DIR/${test_name}.link.err"; then
        print_status "FAIL" "$test_name - Linking failed"
        cat "$TEMP_DIR/${test_name}.link.err"
        cat "$TEMP_DIR/${test_name}.link.err" >> "$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi

    echo "  [4/4] Running executable..."
    if "$exe_file" > "$TEMP_DIR/${test_name}.output" 2>&1; then
        # Check if output contains ERROR: messages (indicating test failures)
        if grep -q "^.*ERROR:" "$TEMP_DIR/${test_name}.output"; then
            print_status "FAIL" "$test_name - Test assertions failed (found ERROR: in output)"
            echo "    Output:" | tee -a "$LOG_FILE"
            cat "$TEMP_DIR/${test_name}.output" | sed 's/^/      /' | tee -a "$LOG_FILE"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        else
            print_status "PASS" "$test_name"
            echo "    Output:" | tee -a "$LOG_FILE"
            cat "$TEMP_DIR/${test_name}.output" | sed 's/^/      /' | tee -a "$LOG_FILE"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        fi
    else
        local exit_code=$?
        print_status "FAIL" "$test_name - Runtime error (exit code: $exit_code)"
        echo "    Output:" | tee -a "$LOG_FILE"
        cat "$TEMP_DIR/${test_name}.output" | sed 's/^/      /' | tee -a "$LOG_FILE"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# Function to test with timeout
test_with_timeout() {
    local bas_file=$1
    local timeout=${2:-10}
    local test_name=$(basename "$bas_file" .bas)

    # Export all necessary variables for the subprocess
    export QBE_BASIC RUNTIME_DIR TEST_DIR TEMP_DIR LOG_FILE
    export TOTAL_TESTS PASSED_TESTS FAILED_TESTS SKIPPED_TESTS
    export RED GREEN YELLOW BLUE NC

    if timeout "${timeout}s" bash -c "$(declare -f test_basic_file print_status); test_basic_file '$bas_file'" 2>&1; then
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            print_status "FAIL" "$test_name - Test timeout (>${timeout}s)"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
        return 1
    fi
}

# Check if qbe_basic exists
if [ ! -f "$QBE_BASIC" ]; then
    echo -e "${RED}Error: qbe_basic not found at $QBE_BASIC${NC}"
    echo "Please run: bash qbe_basic_integrated/build_qbe_basic.sh"
    exit 1
fi

# Check if runtime exists
if [ ! -d "$RUNTIME_DIR" ]; then
    echo -e "${RED}Error: Runtime directory not found at $RUNTIME_DIR${NC}"
    exit 1
fi

echo "╔════════════════════════════════════════════════════════════╗"
echo "║         FasterBASIC Compiler Test Suite                   ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Compiler: $QBE_BASIC"
echo "Runtime:  $RUNTIME_DIR"
echo "Test Dir: $TEST_DIR"
echo "Temp Dir: $TEMP_DIR"
echo ""

# Core tests - simple functionality
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  CORE TESTS - Basic Functionality"
echo "═══════════════════════════════════════════════════════════"

test_basic_file "$TEST_DIR/test_hello.bas"
test_basic_file "$TEST_DIR/test_simple.bas"
test_basic_file "$TEST_DIR/test_if.bas"
test_basic_file "$TEST_DIR/test_ifelse.bas"
test_basic_file "$TEST_DIR/test_goto.bas"

# Arithmetic tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ARITHMETIC TESTS"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/arithmetic/test_integer_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_integer_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arithmetic/test_double_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_double_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arithmetic/test_power_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_power_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arithmetic/test_intdiv_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_intdiv_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arithmetic/test_not_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_not_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arithmetic/test_bitwise_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_bitwise_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arithmetic/test_mod_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_mod_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arithmetic/test_mixed_types.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_mixed_types.bas"
fi

# Comparison and logical tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  COMPARISON & LOGICAL TESTS"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/conditionals/test_comparisons.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/conditionals/test_comparisons.bas"
fi
if [ -f "$SCRIPT_DIR/tests/conditionals/test_logical.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/conditionals/test_logical.bas"
fi
if [ -f "$SCRIPT_DIR/tests/conditionals/test_select_case.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/conditionals/test_select_case.bas"
fi
if [ -f "$SCRIPT_DIR/tests/comparisons/test_numeric_comparisons.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/comparisons/test_numeric_comparisons.bas"
fi


# Loop tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  LOOP TESTS"
echo "═══════════════════════════════════════════════════════════"

test_basic_file "$TEST_DIR/test_while.bas"
test_basic_file "$TEST_DIR/test_do_plain.bas"
test_basic_file "$TEST_DIR/test_do_loop.bas"
test_basic_file "$TEST_DIR/test_exit_for_simple.bas"
test_basic_file "$TEST_DIR/test_exit_for.bas"

if [ -f "$SCRIPT_DIR/tests/loops/test_for_comprehensive.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/loops/test_for_comprehensive.bas"
fi
if [ -f "$SCRIPT_DIR/tests/loops/test_while_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/loops/test_while_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/loops/test_do_comprehensive.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/loops/test_do_comprehensive.bas"
fi
if [ -f "$SCRIPT_DIR/tests/loops/test_exit_statements.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/loops/test_exit_statements.bas"
fi

# CFG / Nested Control Flow Tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  CFG TESTS - Nested Control Flow (Bug Fix Verification)"
echo "═══════════════════════════════════════════════════════════"

# Test WHILE loops nested inside IF statements (previously broken)
if [ -f "$SCRIPT_DIR/tests/test_while_if_nested.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/test_while_if_nested.bas"
fi

# Test FOR loops nested inside IF statements
if [ -f "$SCRIPT_DIR/tests/test_for_if_nested.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/test_for_if_nested.bas"
fi

# Test simple nested WHILE (baseline - should always work)
if [ -f "$SCRIPT_DIR/tests/test_while_nested_simple.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/test_while_nested_simple.bas"
fi

# Test prime sieve (real-world example)
if [ -f "$SCRIPT_DIR/tests/test_primes_sieve.bas" ]; then
    print_status "INFO" "Prime sieve may take a few seconds..."
    test_with_timeout "$SCRIPT_DIR/tests/test_primes_sieve.bas" 10
fi

# Test working sieve (workaround version for comparison)
if [ -f "$SCRIPT_DIR/tests/test_primes_sieve_working.bas" ]; then
    print_status "INFO" "Testing workaround version of prime sieve..."
    test_with_timeout "$SCRIPT_DIR/tests/test_primes_sieve_working.bas" 10
fi

# Function tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  FUNCTION TESTS"
echo "═══════════════════════════════════════════════════════════"

test_basic_file "$TEST_DIR/test_func_simple.bas"
if [ -f "$TEST_DIR/test_factorial.bas" ]; then
    test_basic_file "$TEST_DIR/test_factorial.bas"
fi
if [ -f "$TEST_DIR/test_fibonacci.bas" ]; then
    test_basic_file "$TEST_DIR/test_fibonacci.bas"
fi
if [ -f "$SCRIPT_DIR/tests/functions/test_math_intrinsics.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/functions/test_math_intrinsics.bas"
fi
if [ -f "$SCRIPT_DIR/tests/functions/test_gosub.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/functions/test_gosub.bas"
fi
if [ -f "$SCRIPT_DIR/tests/functions/test_on_goto.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/functions/test_on_goto.bas"
fi
if [ -f "$SCRIPT_DIR/tests/functions/test_on_gosub.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/functions/test_on_gosub.bas"
fi
# Type tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  TYPE TESTS - Critical for QBE Backend"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$TEST_DIR/test_types.bas" ]; then
    test_basic_file "$TEST_DIR/test_types.bas"
fi
if [ -f "$TEST_DIR/test_def_clean.bas" ]; then
    test_basic_file "$TEST_DIR/test_def_clean.bas"
fi
if [ -f "$TEST_DIR/test_def_typed.bas" ]; then
    test_basic_file "$TEST_DIR/test_def_typed.bas"
fi
if [ -f "$TEST_DIR/test_simple_def.bas" ]; then
    test_basic_file "$TEST_DIR/test_simple_def.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_conversions.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_conversions.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_edge_cases.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_edge_cases.bas"
fi

# UDT (User-Defined Type) tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  USER-DEFINED TYPE (UDT) TESTS"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/types/test_udt_simple.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_udt_simple.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_udt_twofields.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_udt_twofields.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_udt_nested.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_udt_nested.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_udt_string.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_udt_string.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_udt_array.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_udt_array.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_udt_long.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_udt_long.bas"
fi

# String tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  STRING TESTS"
echo "═══════════════════════════════════════════════════════════"

test_basic_file "$TEST_DIR/test_string_simple.bas"
test_basic_file "$TEST_DIR/test_concat.bas"
test_basic_file "$TEST_DIR/test_len_simple.bas"
test_basic_file "$TEST_DIR/test_mid.bas"
if [ -f "$SCRIPT_DIR/tests/strings/test_string_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/strings/test_string_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/strings/test_string_functions.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/strings/test_string_functions.bas"
fi
if [ -f "$SCRIPT_DIR/tests/strings/test_string_compare.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/strings/test_string_compare.bas"
fi
if [ -f "$SCRIPT_DIR/tests/strings/test_string_runtime_functions.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/strings/test_string_runtime_functions.bas"
fi

# String type coercion tests (DETECTSTRING mode, ASCII/UNICODE promotion)
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  STRING TYPE COERCION TESTS (DETECTSTRING)"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/types/test_detectstring_simple.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_detectstring_simple.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_detectstring.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_detectstring.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_string_coercion_simple.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_string_coercion_simple.bas"
fi
if [ -f "$SCRIPT_DIR/tests/types/test_string_coercion.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/types/test_string_coercion.bas"
fi

# Array tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ARRAY TESTS"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/arrays/test_array_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arrays/test_array_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arrays/test_array_2d.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arrays/test_array_2d.bas"
fi

# Dynamic array memory management tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  DYNAMIC ARRAY MEMORY MANAGEMENT (ERASE/REDIM/PRESERVE)"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/arrays/test_erase.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arrays/test_erase.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arrays/test_redim.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arrays/test_redim.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arrays/test_redim_preserve.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arrays/test_redim_preserve.bas"
fi
if [ -f "$SCRIPT_DIR/tests/arrays/test_array_memory.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/arrays/test_array_memory.bas"
fi

# I/O tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  INPUT/OUTPUT TESTS"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/io/test_print_formats.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/io/test_print_formats.bas"
fi

# Intrinsics tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  INTRINSIC FUNCTION TESTS"
echo "═══════════════════════════════════════════════════════════"
if [ -f "$SCRIPT_DIR/tests/data/test_data_simple.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/data/test_data_simple.bas"
fi


if [ -f "$TEST_DIR/test_int_intrinsics.bas" ]; then
    test_basic_file "$TEST_DIR/test_int_intrinsics.bas"
fi
if [ -f "$TEST_DIR/test_intrinsics.bas" ]; then
    test_basic_file "$TEST_DIR/test_intrinsics.bas"
fi

# Select case tests (legacy)
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  SELECT CASE TESTS (Legacy)"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$TEST_DIR/test_select.bas" ]; then
    test_basic_file "$TEST_DIR/test_select.bas"
fi

# Exception handling tests
echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  EXCEPTION HANDLING TESTS (TRY/CATCH/FINALLY/THROW)"
echo "═══════════════════════════════════════════════════════════"

if [ -f "$SCRIPT_DIR/tests/exceptions/test_try_catch_basic.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/exceptions/test_try_catch_basic.bas"
fi
if [ -f "$SCRIPT_DIR/tests/exceptions/test_catch_all.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/exceptions/test_catch_all.bas"
fi
if [ -f "$SCRIPT_DIR/tests/exceptions/test_finally.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/exceptions/test_finally.bas"
fi
if [ -f "$SCRIPT_DIR/tests/exceptions/test_nested_try.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/exceptions/test_nested_try.bas"
fi
if [ -f "$SCRIPT_DIR/tests/exceptions/test_err_erl.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/exceptions/test_err_erl.bas"
fi
if [ -f "$SCRIPT_DIR/tests/exceptions/test_comprehensive.bas" ]; then
    test_basic_file "$SCRIPT_DIR/tests/exceptions/test_comprehensive.bas"
fi

# NOTE: test_throw_no_handler.bas is intentionally excluded from the main suite
# as it tests the error case (THROW without TRY handler, which should terminate)
# Run it manually if you want to test error termination behavior

# Print summary
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    TEST SUMMARY                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Total Tests:   $TOTAL_TESTS"
echo -e "${GREEN}Passed:        $PASSED_TESTS${NC}"
echo -e "${RED}Failed:        $FAILED_TESTS${NC}"
echo -e "${YELLOW}Skipped:       $SKIPPED_TESTS${NC}"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  ✓ ALL TESTS PASSED!${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo "Full log: $LOG_FILE"
    exit 0
else
    echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${RED}  ✗ SOME TESTS FAILED${NC}"
    echo -e "${RED}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo "Full log: $LOG_FILE"
    echo "Failed test outputs are in: $TEMP_DIR"
    exit 1
fi
