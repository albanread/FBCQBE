#!/bin/bash
#
# run_basic.sh
# Compile and run a FasterBASIC program
#
# Usage: ./run_basic.sh program.bas [output_name]
#

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 program.bas [output_name]"
    echo ""
    echo "Compiles and runs a FasterBASIC program through the full toolchain:"
    echo "  1. FasterBASIC -> QBE IL"
    echo "  2. QBE IL -> Assembly"
    echo "  3. Assembly + Runtime -> Executable"
    echo "  4. Run the executable"
    exit 1
fi

BASIC_FILE="$1"
BASE_NAME=$(basename "$BASIC_FILE" .bas)
OUTPUT_NAME="${2:-$BASE_NAME}"

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if input file exists
if [ ! -f "$BASIC_FILE" ]; then
    echo "Error: File '$BASIC_FILE' not found"
    exit 1
fi

echo "=== Compiling FasterBASIC Program ==="
echo "Input: $BASIC_FILE"
echo "Output: $OUTPUT_NAME"
echo ""

# Step 1: Compile BASIC to QBE IL
echo "[1/4] Compiling BASIC to QBE IL..."
./fbc_qbe "$BASIC_FILE" -o "${OUTPUT_NAME}.qbe"

# Step 2: Compile QBE IL to Assembly
echo "[2/4] Compiling QBE IL to Assembly..."
./qbe/qbe "${OUTPUT_NAME}.qbe" > "${OUTPUT_NAME}.s"

# Step 3: Compile Assembly with Runtime to Executable
echo "[3/4] Linking with runtime library..."
cc "${OUTPUT_NAME}.s" \
   FasterBASICT/runtime_c/array_ops.c \
   FasterBASICT/runtime_c/basic_data.c \
   FasterBASICT/runtime_c/basic_runtime.c \
   FasterBASICT/runtime_c/conversion_ops.c \
   FasterBASICT/runtime_c/io_ops.c \
   FasterBASICT/runtime_c/math_ops.c \
   FasterBASICT/runtime_c/memory_mgmt.c \
   FasterBASICT/runtime_c/string_ops.c \
   FasterBASICT/runtime_c/string_utf32.c \
   -IFasterBASICT/runtime_c -o "$OUTPUT_NAME"

# Step 4: Run the executable
echo "[4/4] Running $OUTPUT_NAME..."
echo "=== Program Output ==="
./"$OUTPUT_NAME"
EXIT_CODE=$?

echo ""
echo "=== Program exited with code $EXIT_CODE ==="

# Cleanup intermediate files
if [ "$EXIT_CODE" -eq 0 ]; then
    rm -f "${OUTPUT_NAME}.qbe" "${OUTPUT_NAME}.s"
    echo "Cleaned up intermediate files"
fi

exit $EXIT_CODE
