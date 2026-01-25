# QBE + FasterBASIC Integrated Compiler

Single-binary compiler that accepts both QBE IL and FasterBASIC source files.

## Architecture

```
BASIC source (.bas)
    ↓
FasterBASIC Frontend → QBE IL (in memory)
    ↓
QBE Backend → Assembly (.s)
    ↓
Clang/GCC → Executable
```

## Features

- **Single pass compilation**: BASIC → assembly in memory (no temp files)
- **Unified binary**: One tool for both QBE IL and BASIC
- **Fast**: No process spawning overhead between stages
- **Drop-in compatible**: Works with existing QBE workflows

## Building

```bash
./build.sh
```

This creates `qbe_basic` executable.

## Usage

```bash
# Compile BASIC source
./qbe_basic program.bas -o program.s

# Compile QBE IL (still works)
./qbe_basic program.qbe -o program.s

# Link with runtime
clang program.s ../fsh/FasterBASICT/runtime_c/*.c -o program
```

## Implementation

- Modified `qbe/main.c` to detect `.bas` files
- Added `basic_frontend.c` that runs FasterBASIC compiler
- Uses `fmemopen()` to pass IL in memory to QBE parser
- Zero changes to QBE's optimization pipeline
