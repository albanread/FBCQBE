#!/bin/bash
#
# build_fbc_qbe.sh
# Build the FasterBASIC QBE Compiler (simplified version with modular codegen)
#

set -e

echo "=== Building FasterBASIC QBE Compiler ==="
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Set up paths
SRC_DIR="FasterBASICT/src"
CODEGEN_DIR="$SRC_DIR/codegen"
RUNTIME_DIR="FasterBASICT/runtime"
BUILD_DIR="FasterBASICT/build_qbe"

# Create build directory
mkdir -p "$BUILD_DIR"

echo "Compiling source files..."
echo ""

# Compile lexer
echo "  [1/13] Compiling fasterbasic_lexer.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$SRC_DIR/fasterbasic_lexer.cpp" \
    -o "$BUILD_DIR/fasterbasic_lexer.o"

# Compile parser
echo "  [2/13] Compiling fasterbasic_parser.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$SRC_DIR/fasterbasic_parser.cpp" \
    -o "$BUILD_DIR/fasterbasic_parser.o"

# Compile semantic analyzer
echo "  [3/13] Compiling fasterbasic_semantic.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$SRC_DIR/fasterbasic_semantic.cpp" \
    -o "$BUILD_DIR/fasterbasic_semantic.o"

# Compile CFG builder
echo "  [4/13] Compiling fasterbasic_cfg.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$SRC_DIR/fasterbasic_cfg.cpp" \
    -o "$BUILD_DIR/fasterbasic_cfg.o"

# Compile data preprocessor
echo "  [5/13] Compiling fasterbasic_data_preprocessor.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$SRC_DIR/fasterbasic_data_preprocessor.cpp" \
    -o "$BUILD_DIR/fasterbasic_data_preprocessor.o"

# Compile modular QBE code generator files
echo "  [6/13] Compiling qbe_codegen_main.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$CODEGEN_DIR/qbe_codegen_main.cpp" \
    -o "$BUILD_DIR/qbe_codegen_main.o"

echo "  [7/13] Compiling qbe_codegen_expressions.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$CODEGEN_DIR/qbe_codegen_expressions.cpp" \
    -o "$BUILD_DIR/qbe_codegen_expressions.o"

echo "  [8/13] Compiling qbe_codegen_statements.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$CODEGEN_DIR/qbe_codegen_statements.cpp" \
    -o "$BUILD_DIR/qbe_codegen_statements.o"

echo "  [9/13] Compiling qbe_codegen_runtime.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$CODEGEN_DIR/qbe_codegen_runtime.cpp" \
    -o "$BUILD_DIR/qbe_codegen_runtime.o"

echo "  [10/13] Compiling qbe_codegen_helpers.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$CODEGEN_DIR/qbe_codegen_helpers.cpp" \
    -o "$BUILD_DIR/qbe_codegen_helpers.o"

# Compile modular commands
echo "  [11/13] Compiling modular_commands.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$SRC_DIR/modular_commands.cpp" \
    -o "$BUILD_DIR/modular_commands.o"

# Compile command registry core
echo "  [12/13] Compiling command_registry_core.cpp..."
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    "$SRC_DIR/command_registry_core.cpp" \
    -o "$BUILD_DIR/command_registry_core.o"

echo ""
echo "Compiling runtime dependencies..."

# Compile ConstantsManager
echo "  - ConstantsManager.cpp"
g++ -std=c++17 -O2 -c \
    -I"$RUNTIME_DIR" \
    "$RUNTIME_DIR/ConstantsManager.cpp" \
    -o "$BUILD_DIR/ConstantsManager.o"

echo ""
echo "Compiling main compiler..."

# Compile main fbc_qbe
echo "  [13/13] fbc_qbe.cpp"
g++ -std=c++17 -O2 -c \
    -I"$SRC_DIR" \
    -I"$RUNTIME_DIR" \
    "$SRC_DIR/fbc_qbe.cpp" \
    -o "$BUILD_DIR/fbc_qbe.o"

echo ""
echo "Linking fbc_qbe executable..."

# Link everything together
g++ -std=c++17 -O2 \
    "$BUILD_DIR/fbc_qbe.o" \
    "$BUILD_DIR/fasterbasic_lexer.o" \
    "$BUILD_DIR/fasterbasic_parser.o" \
    "$BUILD_DIR/fasterbasic_semantic.o" \
    "$BUILD_DIR/fasterbasic_cfg.o" \
    "$BUILD_DIR/fasterbasic_data_preprocessor.o" \
    "$BUILD_DIR/qbe_codegen_main.o" \
    "$BUILD_DIR/qbe_codegen_expressions.o" \
    "$BUILD_DIR/qbe_codegen_statements.o" \
    "$BUILD_DIR/qbe_codegen_runtime.o" \
    "$BUILD_DIR/qbe_codegen_helpers.o" \
    "$BUILD_DIR/modular_commands.o" \
    "$BUILD_DIR/command_registry_core.o" \
    "$BUILD_DIR/ConstantsManager.o" \
    -o fbc_qbe

echo ""
echo "=== Build Complete ==="
echo "QBE compiler: $SCRIPT_DIR/fbc_qbe"
echo ""
echo "Modular codegen files:"
echo "  - qbe_codegen_main.cpp        (orchestration & block emission)"
echo "  - qbe_codegen_expressions.cpp (expression emission)"
echo "  - qbe_codegen_statements.cpp  (statement emission)"
echo "  - qbe_codegen_runtime.cpp     (runtime library calls)"
echo "  - qbe_codegen_helpers.cpp     (utility functions)"
echo ""
echo "Test with:"
echo "  ./fbc_qbe --verbose test_hello.bas"
echo ""
