# FasterBASIC QBE Runtime - Linking Strategy

## Overview

This document describes how the C/C++ runtime library is tightly bound to QBE-generated BASIC executables through static linking.

## Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                     Build Once: Runtime Library                 │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  FasterBASICT/runtime_c/                                       │
│  ├── basic_runtime.h          (API declarations)              │
│  ├── basic_runtime.c          (initialization, cleanup)       │
│  ├── string_ops.c             (string management)             │
│  ├── array_ops.c              (array operations)              │
│  ├── io_ops.c                 (PRINT, INPUT, files)           │
│  ├── memory_mgmt.c            (ref counting, arena)           │
│  ├── math_ops.c               (math functions)                │
│  └── conversion_ops.c         (type conversions)              │
│           ↓                                                     │
│      [Compile]                                                  │
│           ↓                                                     │
│  FasterBASICT/lib/libbasic_runtime.a   (50-200 KB)           │
│                                                                 │
└────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│              For Each BASIC Program: Compile & Link             │
├────────────────────────────────────────────────────────────────┤
│                                                                 │
│  program.bas                                                    │
│       ↓                                                         │
│  fbc --target=qbe program.bas                                  │
│       ↓                                                         │
│  program.ssa (QBE IL with runtime calls)                       │
│       ↓                                                         │
│  qbe program.ssa > program.s                                   │
│       ↓                                                         │
│  program.s (Assembly)                                          │
│       ↓                                                         │
│  cc program.s libbasic_runtime.a -o program                    │
│       ↓                                                         │
│  program (standalone executable, no external dependencies)     │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

## Static Linking Strategy

### Why Static Linking?

1. **Single Executable**: No need to distribute separate runtime libraries
2. **No Version Conflicts**: Runtime is embedded, no DLL/shared library issues
3. **Fast Startup**: No dynamic linking overhead
4. **Portable**: Executable runs anywhere (same OS/arch)
5. **Optimization**: Linker can optimize across boundaries with LTO

### Disadvantages (Acceptable for our use case)

1. **Larger Executables**: ~50-200 KB overhead per executable
2. **No Shared Memory**: Each program has its own runtime copy
3. **Updates Require Recompile**: Can't update runtime without recompiling programs

## Build Process

### Step 1: Build Runtime Library (Once)

```bash
cd FasterBASICT/runtime_c
make

# Produces:
# FasterBASICT/lib/libbasic_runtime.a
```

This creates a static archive containing all runtime object files.

### Step 2: Compile BASIC Program

```bash
# Generate QBE IL
./fbc --target=qbe program.bas -o program.ssa

# Compile QBE IL to assembly
./qbe/qbe program.ssa > program.s

# Assemble and link with runtime
cc program.s FasterBASICT/lib/libbasic_runtime.a -o program -lm

# Result: Standalone executable
./program
```

### Step 3: Distribution

The generated executable (`program`) is completely standalone:
- No need to distribute libbasic_runtime.a
- No need to install anything on target system
- Just copy and run

## QBE IL Runtime Calls

The QBE code generator emits calls to runtime functions:

### Example: BASIC String Operations

```basic
10 A$ = "Hello"
20 B$ = " World"
30 C$ = A$ + B$
40 PRINT C$
```

### Generated QBE IL

```ssa
data $str_hello = { b "Hello", b 0 }
data $str_world = { b " World", b 0 }

export function w $main() {
@start
    # Initialize runtime
    call $basic_runtime_init()
    
    # A$ = "Hello"
    %a_str =l call $str_new(l $str_hello)
    
    # B$ = " World"
    %b_str =l call $str_new(l $str_world)
    
    # C$ = A$ + B$
    %c_str =l call $str_concat(l %a_str, l %b_str)
    
    # PRINT C$
    call $basic_print_string(l %c_str)
    call $basic_print_newline()
    
    # Cleanup
    call $str_release(l %a_str)
    call $str_release(l %b_str)
    call $str_release(l %c_str)
    call $basic_runtime_cleanup()
    
    ret 0
}
```

### Linking Process

```bash
# QBE compiles to assembly with external symbols
./qbe/qbe program.ssa > program.s

# Assembly contains:
#   bl _str_new
#   bl _str_concat
#   bl _basic_print_string
#   ... etc

# Linker resolves these from libbasic_runtime.a
cc program.s libbasic_runtime.a -o program -lm
#                                           ^^^ math library for sqrt, sin, etc.

# Result: All symbols resolved, single executable
```

## Symbol Resolution

### Runtime Exports (from libbasic_runtime.a)

The static library exports all public symbols:

```c
// String operations
BasicString* str_new(const char* cstr);
BasicString* str_concat(BasicString* a, BasicString* b);
void str_release(BasicString* str);

// I/O operations
void basic_print_string(BasicString* str);
void basic_print_int(int32_t value);
void basic_print_newline(void);

// Array operations
BasicArray* array_new(char type, int32_t dims, int32_t* bounds, int32_t base);
int32_t array_get_int(BasicArray* array, int32_t* indices);

// Math operations
double basic_sqrt(double x);
double basic_sin(double x);
// ... etc
```

### Program Imports (from QBE-generated code)

The QBE-generated assembly imports these symbols:

```asm
.text
.globl _main
_main:
    ; Call runtime function
    bl _str_new           ; Imported from libbasic_runtime.a
    bl _basic_print_string
    bl _str_release
    ret
```

### Linker Job

The linker (`cc` or `ld`) connects imports to exports:

```
program.o:      str_new = UNDEFINED
libbasic_runtime.a(string_ops.o):  str_new = DEFINED

Linker: RESOLVE str_new -> string_ops.o:str_new
```

## Platform-Specific Notes

### macOS (both Intel and Apple Silicon)

```bash
# Build runtime
cd FasterBASICT/runtime_c
make

# Compile BASIC program
cc program.s FasterBASICT/lib/libbasic_runtime.a -o program -lm

# QBE automatically detects architecture:
# - arm64_apple for Apple Silicon
# - amd64_apple for Intel
```

### Linux (x64, ARM64, RISC-V)

```bash
# Build runtime
cd FasterBASICT/runtime_c
make

# Compile BASIC program
cc program.s FasterBASICT/lib/libbasic_runtime.a -o program -lm

# Specify QBE target if needed:
./qbe/qbe -t arm64 program.ssa > program.s    # ARM64
./qbe/qbe -t amd64_sysv program.ssa > program.s  # x64
./qbe/qbe -t rv64 program.ssa > program.s     # RISC-V
```

## Advanced: Link-Time Optimization (LTO)

For maximum performance, we can use LTO:

```bash
# Build runtime with LTO
cd FasterBASICT/runtime_c
make CFLAGS="-std=c11 -Wall -Wextra -O2 -flto"

# Link program with LTO
cc -flto program.s FasterBASICT/lib/libbasic_runtime.a -o program -lm
```

This allows the compiler to optimize across the boundary between generated code and runtime.

## Debugging

### With Debug Symbols

```bash
# Build runtime with debug symbols
cd FasterBASICT/runtime_c
make CFLAGS="-std=c11 -Wall -Wextra -g -O0"

# Compile program with debug symbols
cc -g program.s FasterBASICT/lib/libbasic_runtime.a -o program -lm

# Debug with lldb/gdb
lldb program
(lldb) breakpoint set -n str_concat
(lldb) run
```

### Viewing Linked Symbols

```bash
# List all symbols in executable
nm program

# List only runtime symbols
nm program | grep basic_

# Check library contents
nm FasterBASICT/lib/libbasic_runtime.a
```

## Build Script Integration

The FBC compiler will handle this automatically:

```bash
./fbc --target=qbe program.bas -o program
```

Internally, FBC will:
1. Generate QBE IL
2. Invoke QBE compiler
3. Invoke assembler
4. Link with runtime library
5. Produce final executable

Users don't need to know about the linking details!

## Size Analysis

Typical executable sizes:

```
Empty BASIC program:
  QBE code:        ~1 KB
  Runtime library: ~50-200 KB (with --gc-sections)
  Total:           ~51-201 KB

Simple program:
  QBE code:        ~5 KB
  Runtime library: ~80 KB (only used functions linked)
  Total:           ~85 KB

Complex program:
  QBE code:        ~50 KB
  Runtime library: ~150 KB
  Total:           ~200 KB
```

### Reducing Size

```bash
# Strip debug symbols
strip program

# Link with garbage collection (remove unused functions)
cc -Wl,--gc-sections program.s libbasic_runtime.a -o program -lm

# Result: Smaller executables (only used functions included)
```

## Future: Shared Library Option

If we later want to support shared libraries:

```bash
# Build shared library
cd FasterBASICT/runtime_c
make shared

# This would produce:
# - libbasic_runtime.so (Linux)
# - libbasic_runtime.dylib (macOS)

# Link dynamically
cc program.s -L../lib -lbasic_runtime -o program
```

But for now, static linking is simpler and more portable.

## Summary

The runtime is **tightly bound** through:

1. **Static linking**: Runtime code embedded in each executable
2. **Single library**: All runtime functions in one `.a` file
3. **Direct calls**: QBE IL calls runtime functions directly (no indirection)
4. **No dependencies**: Final executable is standalone
5. **Optimizable**: LTO can optimize across boundaries

This gives us the best balance of simplicity, portability, and performance.