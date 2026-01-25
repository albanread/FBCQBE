# FasterBASIC QBE Backend

A native code compiler backend for FasterBASIC using QBE (Quick Backend).

## Overview

This project extends FasterBASIC with a QBE-based code generator that produces native executables. While the original FasterBASIC uses LuaJIT for JIT compilation, this fork adds ahead-of-time (AOT) compilation to native machine code.

## Architecture

```
BASIC Source Code
    ↓
Lexer → Tokens
    ↓
Parser → Abstract Syntax Tree (AST)
    ↓
Semantic Analysis
    ↓
Control Flow Graph (CFG)
    ↓
QBE Code Generator (NEW!)
    ↓
QBE Intermediate Language (.ssa)
    ↓
QBE Compiler → Assembly (.s)
    ↓
Assembler → Object File (.o)
    ↓
Linker + Runtime Library → Native Executable
```

## Components

### Frontend (Reused from FasterBASIC)
- **Lexer** (`fasterbasic_lexer.cpp/h`) - Tokenization
- **Parser** (`fasterbasic_parser.cpp/h`) - AST generation
- **Semantic Analyzer** (`fasterbasic_semantic.cpp/h`) - Type checking, symbol resolution
- **CFG Builder** (`fasterbasic_cfg.cpp/h`) - Control flow analysis

### Backend (New)
- **QBE Code Generator** (`fasterbasic_qbe_codegen.cpp/h`) - Generates QBE IL
- **Runtime Library** (`runtime/basic_runtime.c`) - String, array, I/O operations
- **Compiler Driver** (`fbc_qbe.cpp`) - Orchestrates compilation pipeline

## Goals

1. **Native Executables**: Produce standalone binaries with no runtime dependencies (except libc)
2. **Fast Compilation**: Leverage QBE's speed for quick compilation times
3. **Portability**: Support x64, ARM64, and RISC-V architectures via QBE
4. **No JIT Required**: Suitable for environments where JIT is not available or desired

## Status

🚧 **Under Development** 🚧

- [x] QBE code generator
- [x] Runtime library
- [x] Basic data types (integers, floats, strings)
- [x] Control flow (IF, FOR, WHILE, GOTO, GOSUB, ON GOTO, ON GOSUB)
- [ ] Arrays
- [ ] Functions and subroutines
- [ ] I/O operations (PRINT, INPUT, file operations)
- [ ] String operations
- [ ] Build system integration

## Building

### Prerequisites

```bash
# Install QBE
# macOS (via Homebrew)
brew install qbe

# Linux
# Build from source: https://c9x.me/compile/
git clone git://c9x.me/qbe.git
cd qbe
make
sudo make install
```

### Build FBC QBE Compiler

```bash
./rebuild_fbc.sh
```

## Usage (Planned)

```bash
# Compile BASIC program to native executable
./fbc --target=qbe program.bas -o program

# Run the native executable
./program

# Generate QBE IL only (for debugging)
./fbc --target=qbe --emit-qbe program.bas -o program.ssa

# Verbose output
./fbc --target=qbe -v program.bas -o program
```

## Example

```basic
10 REM Simple FasterBASIC program
20 PRINT "Hello from native code!"
30 FOR I = 1 TO 10
40   PRINT "Count: "; I
50 NEXT I
60 END
```

Compiles to native code via QBE IL:

```ssa
export function w $main() {
@start
    %msg =l call $str_new(l $str_0)
    call $basic_print(l %msg)
    call $basic_print_newline()
    
    %i =l alloc4 4
    storew 1, %i
@loop_1_check
    %i_val =w loadw %i
    %cmp =w clesw %i_val, 10
    jnz %cmp, @loop_1_body, @loop_1_end
@loop_1_body
    # ... loop body ...
    jmp @loop_1_check
@loop_1_end
    ret 0
}
```

## Differences from Original FasterBASIC

| Feature | Original (LuaJIT) | QBE Backend |
|---------|-------------------|-------------|
| Execution Model | JIT compilation | AOT compilation |
| Output | Lua source → LuaJIT | QBE IL → Native code |
| Runtime | LuaJIT + Lua runtime | Minimal C runtime |
| Startup Time | ~10-50ms | <1ms |
| Peak Performance | Very fast (JIT) | Fast (native) |
| Memory Management | Lua GC | Reference counting |
| Portability | Requires LuaJIT | Standalone binary |

## Technical Challenges

### Strings
QBE has no native string support. We use:
- C-style `char*` pointers
- Reference counting for memory management
- Runtime library for string operations

### Arrays
Dynamic arrays implemented via:
- `malloc`/`free` wrappers
- Bounds checking in runtime
- Multi-dimensional support

### Memory Management
No garbage collector, using:
- Reference counting for strings
- Explicit allocation/deallocation for arrays
- Arena allocator for temporaries

### Dynamic Features
BASIC features like `ON GOTO` and `ON GOSUB`:
- Implemented as chained conditional jumps in QBE IL
- ON GOSUB uses a return stack for subroutine calls
- CFG-aware fallthrough resolution for correct return targets
- See `../ON_STATEMENTS_DOCUMENTATION.md` for detailed implementation

## License

MIT License - see LICENSE file

Based on FasterBASIC by albanread
QBE backend additions © 2025

## Contributing

This is an experimental project. Contributions welcome!

## Links

- [QBE Compiler](https://c9x.me/compile/)
- [Original FasterBASIC](https://github.com/albanread/fsh)
- QBE IL Documentation: https://c9x.me/compile/doc/il.html