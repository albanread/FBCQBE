# FasterBASIC QBE Backend - Project Status

## Overview

This project adds a QBE (Quick Backend) code generator to FasterBASIC, enabling ahead-of-time (AOT) compilation to native executables. This complements the existing LuaJIT-based JIT compilation system.

## Project Goals

1. ✅ Fork FasterBASIC and set up clean repository
2. ✅ Integrate QBE compiler backend
3. 🚧 Implement QBE IL code generator
4. 🚧 Create C runtime library
5. ⏳ Update build system
6. ⏳ Add comprehensive tests
7. ⏳ Documentation and examples

## Current Status

### ✅ Completed

#### Repository Setup (2025-01-24)
- Cloned original FasterBASIC from albanread/fsh
- Removed connection to upstream repository
- Created fresh local git repository
- Cleaned up unnecessary components:
  - Removed interactive shell (fbsh) code
  - Removed CI/CD workflows
  - Removed example programs
  - Removed documentation for interactive features
- Updated README with QBE-specific goals and architecture

#### QBE Integration (2025-01-24)
- Vendored QBE compiler source code from git://c9x.me/qbe.git
- Built QBE successfully on macOS (ARM64)
- Verified QBE compilation pipeline works end-to-end
- Created test_qbe_hello.ssa reference example
- Documented QBE IL format and integration strategy

### 🚧 In Progress

#### Code Generator Design
- Analyzing existing AST and CFG structures
- Designing QBE IL emission strategy
- Planning runtime library interface

### ⏳ Not Started

#### QBE Code Generator (`FasterBASICT/src/fasterbasic_qbe_codegen.cpp/h`)
- [ ] Basic infrastructure and class structure
- [ ] Expression code generation
- [ ] Statement code generation
- [ ] Control flow (IF, FOR, WHILE, GOTO, GOSUB)
- [ ] Function and subroutine calls
- [ ] Array operations
- [ ] String operations
- [ ] Type conversions
- [ ] I/O operations

#### Runtime Library (`FasterBASICT/runtime/basic_runtime.c/h`)
- [ ] String management (reference counting)
- [ ] Array management (dynamic allocation)
- [ ] Memory management (arena allocator for temps)
- [ ] I/O functions (PRINT, INPUT, file operations)
- [ ] Type conversion functions
- [ ] Math functions
- [ ] String manipulation functions

#### Compiler Driver Updates (`FasterBASICT/src/fbc.cpp`)
- [ ] Add `--target=qbe` command-line flag
- [ ] Integrate QBE code generator into pipeline
- [ ] Handle QBE IL file generation
- [ ] Invoke QBE compiler
- [ ] Invoke assembler and linker
- [ ] Link with runtime library

#### Build System
- [ ] Update `rebuild_fbc.sh` to include QBE
- [ ] Add runtime library compilation
- [ ] Add linking steps
- [ ] Support multiple architectures (x64, ARM64)
- [ ] Linux build support

#### Testing
- [ ] Basic arithmetic and expressions
- [ ] Control flow constructs
- [ ] String operations
- [ ] Array operations
- [ ] Function and subroutine calls
- [ ] I/O operations
- [ ] File operations
- [ ] Complex programs (mandelbrot, prime sieve, etc.)

#### Documentation
- [ ] API documentation for runtime library
- [ ] Code generator architecture documentation
- [ ] User guide for compiling with QBE backend
- [ ] Comparison guide (LuaJIT vs QBE)
- [ ] Troubleshooting guide

## Repository Structure

```
fsh/
├── README.md                          # Project overview (QBE-focused)
├── STATUS.md                          # This file
├── QBE_INTEGRATION.md                 # QBE integration notes
├── LICENSE                            # MIT License
├── BUILD_INFO.txt                     # Build information
├── MANIFEST.txt                       # File manifest
├── test_qbe_hello.ssa                 # QBE test example
├── rebuild_fbc.sh                     # Build script (needs updating)
├── rebuild_fbc_linux.sh               # Linux build script (needs updating)
├── qbe/                               # QBE compiler (vendored)
│   ├── qbe                            # QBE executable
│   ├── doc/                           # QBE documentation
│   ├── test/                          # QBE test suite
│   └── [source files...]
└── FasterBASICT/
    ├── src/                           # Compiler source
    │   ├── fasterbasic_lexer.*        # Lexer (reused)
    │   ├── fasterbasic_parser.*       # Parser (reused)
    │   ├── fasterbasic_ast.h          # AST definitions (reused)
    │   ├── fasterbasic_semantic.*     # Semantic analysis (reused)
    │   ├── fasterbasic_cfg.*          # Control flow graph (reused)
    │   ├── fasterbasic_ircode.*       # IR (for reference)
    │   ├── fasterbasic_lua_codegen.*  # Lua codegen (for reference)
    │   ├── fasterbasic_qbe_codegen.*  # QBE codegen (TO BE CREATED)
    │   └── fbc.cpp                    # Compiler driver (needs updates)
    └── runtime/                       # Runtime libraries
        ├── basic_runtime.c/h          # C runtime (TO BE CREATED)
        └── [existing Lua runtime...]
```

## Architecture

### Compilation Pipeline

```
BASIC Source (.bas)
    ↓
Lexer → Tokens
    ↓
Parser → Abstract Syntax Tree (AST)
    ↓
Semantic Analysis → Typed AST + Symbol Table
    ↓
Control Flow Graph (CFG) Builder
    ↓
┌─────────────────────────────┬──────────────────────────────┐
│   Lua Backend (existing)    │   QBE Backend (new)          │
│   for FBSH/JIT              │   for FBC/AOT                │
├─────────────────────────────┼──────────────────────────────┤
│ IR Generator                │ QBE Code Generator           │
│ Lua Code Generator          │                              │
│    ↓                        │    ↓                         │
│ Lua Source                  │ QBE IL (.ssa)                │
│    ↓                        │    ↓                         │
│ LuaJIT Execution            │ QBE Compiler                 │
│                             │    ↓                         │
│                             │ Assembly (.s)                │
│                             │    ↓                         │
│                             │ Assembler + Linker           │
│                             │    ↓                         │
│                             │ Native Executable            │
└─────────────────────────────┴──────────────────────────────┘
```

### Key Design Decisions

1. **Reuse Frontend**: Lexer, Parser, Semantic Analyzer, and CFG builder are reused from FasterBASIC
2. **Direct QBE IL Generation**: Generate QBE IL directly from AST/CFG, not through the existing IR
3. **C Runtime Library**: All high-level operations (strings, arrays, I/O) implemented in C
4. **Reference Counting**: Memory management for strings uses reference counting
5. **Manual Array Management**: Arrays explicitly allocated/deallocated
6. **SSA Form**: QBE IL is in SSA (Static Single Assignment) form

## QBE IL Basics

### Types
- `w` - 32-bit word (int)
- `l` - 64-bit long (int64, pointers)
- `s` - Single-precision float
- `d` - Double-precision float
- `b` - Byte

### Sigils
- `$` - Global names (functions, data)
- `%` - Temporaries (SSA registers)
- `@` - Block labels
- `:` - User-defined types

### Example: BASIC FOR Loop
```basic
FOR I = 1 TO 10
    PRINT I
NEXT I
```

Compiles to:
```ssa
%i_ptr =l alloc4 4          # Allocate I
storew 1, %i_ptr            # I = 1
@loop_check
    %i =w loadw %i_ptr
    %cmp =w cslew %i, 10    # I <= 10?
    jnz %cmp, @loop_body, @loop_end
@loop_body
    %val =w loadw %i_ptr
    call $basic_print_int(w %val)
    %next =w add %val, 1    # I = I + 1
    storew %next, %i_ptr
    jmp @loop_check
@loop_end
```

## Target Platforms

QBE supports:
- **amd64_apple** - x86-64 macOS
- **amd64_sysv** - x86-64 Linux/BSD
- **arm64_apple** - ARM64 macOS (Apple Silicon)
- **arm64** - ARM64 Linux/BSD
- **rv64** - RISC-V 64-bit

## Git History

```
2610722 - Add QBE integration documentation and test
f473209 - Add QBE compiler backend (vendored)
d8b937b - Initial commit: FasterBASIC QBE backend fork
```

## Next Steps

### Immediate (Phase 1)
1. Create `fasterbasic_qbe_codegen.h` with class structure
2. Implement basic expression code generation
3. Create minimal runtime library with PRINT support
4. Generate QBE IL for simple BASIC programs
5. Test end-to-end compilation

### Short-term (Phase 2)
1. Add control flow support (IF, FOR, WHILE, GOTO)
2. Implement integer and float arithmetic
3. Add basic I/O (PRINT, INPUT)
4. Create build system integration

### Medium-term (Phase 3)
1. Add string support with reference counting
2. Add array support with dynamic allocation
3. Add function and subroutine calls
4. Add file I/O operations

### Long-term (Phase 4)
1. Optimize generated code
2. Add comprehensive test suite
3. Performance benchmarking
4. Documentation and examples
5. Linux support

## Questions to Resolve

1. How should we handle dynamic BASIC features like ON GOTO?
2. What's the best strategy for GOSUB/RETURN (call stack vs computed goto)?
3. Should we support all BASIC types or start with a subset?
4. How do we handle OPTION BASE 0 vs OPTION BASE 1 for arrays?
5. Should we support OPTION UNICODE or stick with ASCII strings initially?

## Resources

- Original FasterBASIC: https://github.com/albanread/fsh
- QBE Compiler: https://c9x.me/compile/
- QBE Documentation: https://c9x.me/compile/doc/il.html
- QBE Source: git://c9x.me/qbe.git

## Contact

This is an experimental fork of FasterBASIC focusing on native code generation via QBE.
Original FasterBASIC by albanread.