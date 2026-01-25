#!/bin/bash
# Build QBE with integrated FasterBASIC frontend

set -e

echo "=== Building QBE with FasterBASIC Integration ==="

cd "$(dirname "$0")"

# Compile basic_frontend.c
echo "Compiling FasterBASIC frontend module..."
cc -c -O2 -Wall basic_frontend.c -o basic_frontend.o

# Get existing QBE object files (excluding main.o which we'll rebuild)
echo "Compiling QBE with modified main.c..."
cc -c -O2 -Wall main.c -o main.o

# Build QBE with all its modules
echo "Linking QBE with FasterBASIC support..."
cc -o qbe_basic \
    main.o \
    basic_frontend.o \
    parse.o ssa.o live.o copy.o fold.o simpl.o ifopt.o gcm.o gvn.o \
    mem.o alias.o load.o util.o rega.o emit.o \
    amd64/emit.o amd64/isel.o amd64/sysv.o \
    arm64/emit.o arm64/isel.o arm64/abi.o \
    rv64/emit.o rv64/isel.o rv64/abi.o \
    -lm

echo ""
echo "=== Build Complete ==="
echo "Executable: qbe_basic"
echo ""
echo "Usage:"
echo "  ./qbe_basic input.qbe -o output.s    # Compile QBE IL"
echo "  ./qbe_basic input.bas -o output.s    # Compile BASIC directly!"
echo ""
