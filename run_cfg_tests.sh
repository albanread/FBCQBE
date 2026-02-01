#!/bin/bash

# CFG Test Runner - Systematically test CFG generation on all test programs
# Usage: ./run_cfg_tests.sh [test_file_or_directory]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Directories
TEST_DIR="tests"
QBE_BASIC="./qbe_basic_integrated/qbe_basic"
RESULTS_DIR="cfg_test_results"

# Counters
TOTAL=0
PASSED=0
FAILED=0
ERRORS=0

# Create results directory
mkdir -p "$RESULTS_DIR"

# Log file
LOG_FILE="$RESULTS_DIR/test_run_$(date +%Y%m%d_%H%M%S).log"
SUMMARY_FILE="$RESULTS_DIR/cfg_summary.txt"

echo "========================================" | tee "$LOG_FILE"
echo "CFG Test Runner" | tee -a "$LOG_FILE"
echo "========================================" | tee -a "$LOG_FILE"
echo "Started: $(date)" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

# Function to test a single file
test_file() {
    local file="$1"
    local basename=$(basename "$file" .bas)
    local output_file="$RESULTS_DIR/${basename}_cfg.txt"

    TOTAL=$((TOTAL + 1))

    echo -n "Testing: $file ... " | tee -a "$LOG_FILE"

    # Run the CFG generation (ignore exit code since codegen is disabled)
    $QBE_BASIC -G "$file" > "$output_file" 2>&1 || true

    # Check if CFG was generated (look for "CFG ANALYSIS REPORT")
    if grep -q "CFG ANALYSIS REPORT" "$output_file"; then
        # Extract compact format line
        COMPACT=$(grep "^CFG:" "$output_file" | head -1)

        if [ -z "$COMPACT" ]; then
            echo -e "${RED}FAIL${NC} (no compact format)" | tee -a "$LOG_FILE"
            FAILED=$((FAILED + 1))
        else
            # Check for warnings/issues
            if grep -q "UNREACHABLE" "$output_file"; then
                echo -e "${YELLOW}WARN${NC} (unreachable blocks)" | tee -a "$LOG_FILE"
                echo "  Compact: $COMPACT" | tee -a "$LOG_FILE"
                PASSED=$((PASSED + 1))
            elif grep -q "Orphan" "$output_file"; then
                echo -e "${YELLOW}WARN${NC} (orphan blocks)" | tee -a "$LOG_FILE"
                echo "  Compact: $COMPACT" | tee -a "$LOG_FILE"
                PASSED=$((PASSED + 1))
            else
                echo -e "${GREEN}PASS${NC}" | tee -a "$LOG_FILE"
                echo "  Compact: $COMPACT" | tee -a "$LOG_FILE"
                PASSED=$((PASSED + 1))
            fi

            # Save compact format to summary
            echo "$basename: $COMPACT" >> "$SUMMARY_FILE"
        fi
    else
        # Check if it's a parse/semantic error
        if grep -q "Error:" "$output_file" || grep -q "error:" "$output_file"; then
            echo -e "${RED}ERROR${NC} (parse/semantic)" | tee -a "$LOG_FILE"
            grep -i "error:" "$output_file" | head -3 | sed 's/^/    /' | tee -a "$LOG_FILE"
            ERRORS=$((ERRORS + 1))
        else
            echo -e "${RED}FAIL${NC} (no CFG generated)" | tee -a "$LOG_FILE"
            FAILED=$((FAILED + 1))
        fi
    fi
}

# Function to recursively test directory
test_directory() {
    local dir="$1"

    echo -e "\n${BLUE}Testing directory: $dir${NC}" | tee -a "$LOG_FILE"
    echo "----------------------------------------" | tee -a "$LOG_FILE"

    # Test all .bas files in this directory
    for file in "$dir"/*.bas; do
        if [ -f "$file" ]; then
            test_file "$file"
        fi
    done

    # Recursively test subdirectories
    for subdir in "$dir"/*/; do
        if [ -d "$subdir" ]; then
            test_directory "$subdir"
        fi
    done
}

# Main execution
if [ $# -eq 0 ]; then
    # No arguments - test everything
    echo "Testing all programs in $TEST_DIR" | tee -a "$LOG_FILE"
    test_directory "$TEST_DIR"
elif [ -f "$1" ]; then
    # Single file
    echo "Testing single file: $1" | tee -a "$LOG_FILE"
    test_file "$1"
elif [ -d "$1" ]; then
    # Directory
    echo "Testing directory: $1" | tee -a "$LOG_FILE"
    test_directory "$1"
else
    echo "Error: '$1' not found" | tee -a "$LOG_FILE"
    exit 1
fi

# Summary
echo "" | tee -a "$LOG_FILE"
echo "========================================" | tee -a "$LOG_FILE"
echo "SUMMARY" | tee -a "$LOG_FILE"
echo "========================================" | tee -a "$LOG_FILE"
echo "Total tests:    $TOTAL" | tee -a "$LOG_FILE"
echo -e "${GREEN}Passed:         $PASSED${NC}" | tee -a "$LOG_FILE"
echo -e "${RED}Failed:         $FAILED${NC}" | tee -a "$LOG_FILE"
echo -e "${RED}Errors:         $ERRORS${NC}" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

if [ $FAILED -gt 0 ] || [ $ERRORS -gt 0 ]; then
    echo -e "${YELLOW}Some tests had issues. Check $RESULTS_DIR for details.${NC}" | tee -a "$LOG_FILE"
else
    echo -e "${GREEN}All tests passed!${NC}" | tee -a "$LOG_FILE"
fi

echo "" | tee -a "$LOG_FILE"
echo "Results saved to: $RESULTS_DIR" | tee -a "$LOG_FILE"
echo "Log file: $LOG_FILE" | tee -a "$LOG_FILE"
echo "Summary: $SUMMARY_FILE" | tee -a "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"
echo "Finished: $(date)" | tee -a "$LOG_FILE"

# Exit with error if any tests failed
if [ $FAILED -gt 0 ] || [ $ERRORS -gt 0 ]; then
    exit 1
fi

exit 0
