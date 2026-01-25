#!/bin/bash
# Build integrated QBE + FasterBASIC compiler (all-in-one binary)

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
QBE_DIR="$PROJECT_ROOT/qbe_source"
FASTERBASIC_SRC="$PROJECT_ROOT/../fsh/FasterBASICT/src"

echo "=== Building Integrated QBE + FasterBASIC Compiler ==="
echo "Single binary with embedded FasterBASIC compiler"
echo ""

# Step 1: Compile FasterBASIC sources to object files
echo "[1/3] Compiling FasterBASIC compiler sources..."
cd "$PROJECT_ROOT"

mkdir -p obj

# Compile each FasterBASIC C++ source
clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -I"$FASTERBASIC_SRC/../runtime" -c \
    "$FASTERBASIC_SRC/fasterbasic_lexer.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_parser.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_semantic.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_cfg.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_data_preprocessor.cpp" \
    "$FASTERBASIC_SRC/modular_commands.cpp" \
    "$FASTERBASIC_SRC/command_registry_core.cpp" \
    "$FASTERBASIC_SRC/../runtime/ConstantsManager.cpp" \
    "$FASTERBASIC_SRC/codegen/qbe_codegen_main.cpp" \
    "$FASTERBASIC_SRC/codegen/qbe_codegen_expressions.cpp" \
    "$FASTERBASIC_SRC/codegen/qbe_codegen_statements.cpp" \
    "$FASTERBASIC_SRC/codegen/qbe_codegen_helpers.cpp" \
    "$FASTERBASIC_SRC/codegen/qbe_codegen_runtime.cpp" \
    2>&1 | grep -E "^(.+error:|fatal)" || true

mv *.o obj/ 2>/dev/null || true

echo "  ✓ FasterBASIC compiled"

# Step 1.5: Precompile runtime library
echo "[1.5/3] Precompiling BASIC runtime library..."
RUNTIME_SRC="$PROJECT_ROOT/../fsh/runtime_stubs.c"
if [ -f "$RUNTIME_SRC" ]; then
    cc -std=c99 -O2 -c "$RUNTIME_SRC" -o "$PROJECT_ROOT/obj/runtime_stubs.o"
    echo "  ✓ Runtime library precompiled"
else
    echo "  ⚠ Warning: runtime_stubs.c not found at $RUNTIME_SRC"
fi

# Step 2: Configure QBE
echo "[2/3] Configuring QBE..."
cd "$QBE_DIR"

ARCH=$(uname -m)
OS=$(uname -s)

if [ "$OS" = "Darwin" ]; then
    if [ "$ARCH" = "arm64" ]; then
        DEFAULT_TARGET="T_arm64_apple"
    else
        DEFAULT_TARGET="T_amd64_apple"
    fi
else
    if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
        DEFAULT_TARGET="T_arm64"
    elif [ "$ARCH" = "riscv64" ]; then
        DEFAULT_TARGET="T_rv64"
    else
        DEFAULT_TARGET="T_amd64_sysv"
    fi
fi

cat > config.h << EOF
#define VERSION "qbe+fasterbasic"
#define Deftgt $DEFAULT_TARGET
EOF

echo "  ✓ Target: $DEFAULT_TARGET"

# Step 3: Compile wrapper and link everything
echo "[3/3] Linking QBE + embedded FasterBASIC..."

# Compile wrapper
clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -I"$FASTERBASIC_SRC/../runtime" -I"$QBE_DIR" \
    -c "$PROJECT_ROOT/fasterbasic_wrapper.cpp" \
    -o "$PROJECT_ROOT/obj/fasterbasic_wrapper.o"

clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -I"$FASTERBASIC_SRC/../runtime" -I"$QBE_DIR" \
    -c "$PROJECT_ROOT/basic_frontend.cpp" \
    -o "$PROJECT_ROOT/obj/basic_frontend.o"

# Compile QBE C sources
cd "$QBE_DIR"
cc -std=c99 -O2 -c \
    main.c parse.c ssa.c live.c copy.c fold.c simpl.c ifopt.c gcm.c gvn.c \
    mem.c alias.c load.c util.c rega.c emit.c cfg.c abi.c spill.c \
    2>&1 | grep -E "^(.+error:|fatal)" || true

# Compile architecture-specific sources
(cd amd64 && cc -std=c99 -O2 -c *.c)
(cd arm64 && cc -std=c99 -O2 -c *.c)
(cd rv64 && cc -std=c99 -O2 -c *.c)

# Link everything (use C++ linker for stdlib)
clang++ -O2 -o "$PROJECT_ROOT/qbe_basic" \
    main.o parse.o ssa.o live.o copy.o fold.o simpl.o ifopt.o gcm.o gvn.o \
    mem.o alias.o load.o util.o rega.o emit.o cfg.o abi.o spill.o \
    amd64/*.o \
    arm64/*.o \
    rv64/*.o \
    "$PROJECT_ROOT/obj"/*.o

echo ""
echo "=== Build Complete ==="
echo "Binary: $PROJECT_ROOT/qbe_basic"
echo ""
echo "Single binary with embedded FasterBASIC!"
echo ""
echo "Usage:"
echo "  ./qbe_basic input.bas -o output.s"
echo "  ./qbe_basic input.qbe -o output.s"
echo ""
