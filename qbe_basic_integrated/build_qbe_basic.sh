#!/bin/bash
# Build QBE with integrated FasterBASIC frontend

set -e

echo "=== Building QBE with FasterBASIC Integration ==="

cd "$(dirname "$0")"

PROJECT_ROOT="$(pwd)"
QBE_DIR="$PROJECT_ROOT/qbe_source"
FASTERBASIC_SRC="$PROJECT_ROOT/../fsh/FasterBASICT/src"

# Check if FasterBASIC sources exist
if [ ! -d "$FASTERBASIC_SRC" ]; then
    echo "Error: FasterBASIC sources not found at $FASTERBASIC_SRC"
    exit 1
fi

# Step 1: Compile FasterBASIC sources to object files
echo "Compiling FasterBASIC compiler sources..."

mkdir -p obj

# Compile each FasterBASIC C++ source
# NOTE: Using modular CFG structure (February 2026 refactor)
# NOTE: Codegen disabled - needs adaptation to CFG v2
clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -I"$FASTERBASIC_SRC/../runtime" -c \
    "$FASTERBASIC_SRC/fasterbasic_lexer.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_parser.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_semantic.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_core.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_blocks.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_comprehensive_dump.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_jumptargets.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_statements.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_jumps.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_conditional.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_loops.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_exception.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_functions.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_edges.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_data_preprocessor.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_ast_dump.cpp" \
    "$FASTERBASIC_SRC/modular_commands.cpp" \
    "$FASTERBASIC_SRC/command_registry_core.cpp" \
    "$FASTERBASIC_SRC/../runtime/ConstantsManager.cpp"
#    "$FASTERBASIC_SRC/codegen/qbe_codegen_main.cpp" \
#    "$FASTERBASIC_SRC/codegen/qbe_codegen_statements.cpp" \
#    "$FASTERBASIC_SRC/codegen/qbe_codegen_expressions.cpp" \
#    "$FASTERBASIC_SRC/codegen/qbe_codegen_helpers.cpp" \
#    "$FASTERBASIC_SRC/codegen/qbe_codegen_runtime.cpp"

mv *.o obj/ 2>/dev/null || true

echo "  ✓ FasterBASIC compiled"

# Step 2: Compile runtime library
echo "Compiling BASIC runtime library..."
RUNTIME_SRC="$PROJECT_ROOT/../fsh/runtime_stubs.c"
if [ -f "$RUNTIME_SRC" ]; then
    cc -std=c99 -O2 -c "$RUNTIME_SRC" -o "$PROJECT_ROOT/obj/runtime_stubs.o"
    echo "  ✓ Runtime library compiled"
else
    echo "  ⚠ Warning: runtime_stubs.c not found at $RUNTIME_SRC"
fi

# Step 3: Compile wrapper and frontend
echo "Compiling FasterBASIC wrapper and frontend..."

clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -I"$FASTERBASIC_SRC/../runtime" -I"$QBE_DIR" \
    -c "$PROJECT_ROOT/fasterbasic_wrapper.cpp" \
    -o "$PROJECT_ROOT/obj/fasterbasic_wrapper.o"

clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -I"$FASTERBASIC_SRC/../runtime" -I"$QBE_DIR" \
    -c "$PROJECT_ROOT/basic_frontend.cpp" \
    -o "$PROJECT_ROOT/obj/basic_frontend.o"

echo "  ✓ Wrapper compiled"

# Step 4: Copy runtime files to local directory
echo "Copying runtime files..."
RUNTIME_SRC="$PROJECT_ROOT/../fsh/FasterBASICT/runtime_c"
RUNTIME_DEST="$PROJECT_ROOT/runtime"

mkdir -p "$RUNTIME_DEST"

if [ -d "$RUNTIME_SRC" ]; then
    cp "$RUNTIME_SRC"/*.c "$RUNTIME_DEST/" 2>/dev/null || true
    cp "$RUNTIME_SRC"/*.h "$RUNTIME_DEST/" 2>/dev/null || true
    echo "  ✓ Runtime files copied to runtime/"
else
    echo "  ⚠ Warning: Runtime source not found at $RUNTIME_SRC"
fi

# Step 5: Ensure QBE objects are built
echo "Checking QBE object files..."
cd "$QBE_DIR"

if [ ! -f "main.o" ] || [ ! -f "parse.o" ]; then
    echo "Building QBE object files..."

    # Configure QBE if needed
    if [ ! -f "config.h" ]; then
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
    fi

    # Compile QBE C sources
    cc -std=c99 -O2 -c \
        main.c parse.c ssa.c live.c copy.c fold.c simpl.c ifopt.c gcm.c gvn.c \
        mem.c alias.c load.c util.c rega.c emit.c cfg.c abi.c spill.c

    # Compile architecture-specific sources
    (cd amd64 && cc -std=c99 -O2 -c *.c)
    (cd arm64 && cc -std=c99 -O2 -c *.c)
    (cd rv64 && cc -std=c99 -O2 -c *.c)
fi

echo "  ✓ QBE objects ready"

# Step 6: Link everything together
echo "Linking QBE with FasterBASIC support..."

clang++ -O2 -o "$PROJECT_ROOT/qbe_basic" \
    main.o parse.o ssa.o live.o copy.o fold.o simpl.o ifopt.o gcm.o gvn.o \
    mem.o alias.o load.o util.o rega.o emit.o cfg.o abi.o spill.o \
    amd64/*.o \
    arm64/*.o \
    rv64/*.o \
    "$PROJECT_ROOT/obj"/*.o

echo ""
echo "=== Build Complete ==="
echo "Executable: $PROJECT_ROOT/qbe_basic"
echo ""
echo "Usage:"
echo "  ./qbe_basic input.bas -o program      # Compile to executable"
echo "  ./qbe_basic -i -o output.qbe input.bas # Generate QBE IL only"
echo "  ./qbe_basic -c -o output.s input.bas   # Generate assembly only"
echo ""
echo "Note: Runtime library will be built automatically on first use"
echo "      and cached in runtime/.obj/ for faster subsequent builds."
echo ""
