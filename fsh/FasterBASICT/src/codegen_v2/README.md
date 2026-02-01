# QBE Code Generator V2

This directory contains the new CFG-v2-aware QBE code generator for FasterBASIC.

## Overview

The old code generator assumed sequential block numbering and implicit fallthrough, which is incompatible with CFG v2's explicit edge-based control flow. This new generator is built from the ground up to work with CFG v2's explicit edges and proper control flow semantics.

## Architecture

The code generator is organized into seven modular components:

### 1. **QBEBuilder** (`qbe_builder.{h,cpp}`)
Low-level IL emission. Responsible for:
- Emitting QBE instructions (add, sub, mul, call, ret, etc.)
- Managing temporary variables
- Emitting labels and jumps
- Building the raw QBE IL text

### 2. **TypeManager** (`type_manager.{h,cpp}`)
Type mapping and conversions. Handles:
- BASIC type → QBE type mapping (INTEGER → `w`, SINGLE → `s`, STRING → `l`, etc.)
- Type coercion and conversion instructions
- Array descriptor types
- Return type handling

### 3. **SymbolMapper** (`symbol_mapper.{h,cpp}`)
Name mangling and symbol mapping. Manages:
- Variable name mangling (avoid QBE reserved words, handle BASIC special chars)
- Label generation (blocks, functions, subroutines)
- Scope tracking (global vs. local variables)
- Unique temporary naming

### 4. **RuntimeLibrary** (`runtime_library.{h,cpp}`)
Runtime call wrappers. Provides:
- Intrinsic function calls (PRINT, CHR$, LEN, MID$, etc.)
- Array operations (bounds checking, descriptor access)
- String operations (concat, slice, assignment)
- Math and I/O functions

### 5. **ASTEmitter** (`ast_emitter.{h,cpp}`)
Statement and expression emission. Emits code for:
- Expressions (literals, variables, binary ops, function calls)
- Statements (LET, PRINT, IF, FOR, WHILE, etc.)
- Uses QBEBuilder for low-level emission
- Works with TypeManager for type conversions

### 6. **CFGEmitter** (`cfg_emitter.{h,cpp}`)
CFG-aware block and edge emission. Handles:
- Block traversal (emit all blocks, including UNREACHABLE)
- Edge-based control flow (FALLTHROUGH, CONDITIONAL, MULTIWAY, RETURN)
- Phi nodes and variable merging (if needed)
- Proper terminator emission (jmp, jnz, ret)

### 7. **QBECodeGeneratorV2** (`qbe_codegen_v2.{h,cpp}`)
Main orchestrator. Coordinates:
- Overall code generation flow
- Global declarations (variables, functions, arrays)
- Function/subroutine generation
- Integration with the compiler pipeline

## Implementation Status

### ✅ Completed
- Directory structure created
- README and design documentation
- **QBEBuilder** - Core IL emission component (qbe_builder.{h,cpp}, 273 lines)
  - Function/block structure (start, end, labels)
  - Arithmetic and logic operations
  - Memory operations (load, store, alloc)
  - Control flow (jump, branch, return)
  - Function calls
  - Type conversions
  - Data section and string constants
  - Comments and debugging
- **TypeManager** - Type mapping and conversions (type_manager.{h,cpp}, 293 lines)
  - BASIC to QBE type mapping (BaseType → w/l/s/d)
  - Type size calculations
  - Type checking (numeric, floating point, integral, string)
  - Type promotion rules
  - Conversion operation mapping
  - Default values
- **SymbolMapper** - Name mangling and symbol mapping (symbol_mapper.{h,cpp}, 342 lines)
  - Variable name mangling (handles type suffixes %, $, #, !, &)
  - Array name mangling and descriptors
  - Function/SUB/DEF FN name mangling
  - Label and block name generation
  - String constant naming
  - Scope management (global vs. function scope)
  - QBE reserved word checking and escaping
- **RuntimeLibrary** - Runtime function call wrappers (runtime_library.{h,cpp}, 240 lines)
  - Print/output operations (int, float, double, string, newline, tab)
  - String operations (concat, len, chr$, asc, mid$, left$, right$, compare, assign)
  - Array operations (access, bounds checking, allocation)
  - Math functions (abs, sqr, sin, cos, tan, int, rnd, timer)
  - Input operations (int, float, double, string)
  - Type conversion (str$, val)
  - Control flow helpers (end, runtime error)
- **ASTEmitter** - Statement and expression emission (ast_emitter.{h,cpp}, 639 lines)
  - Expression emission (literals, variables, binary/unary ops, array access, function calls)
  - Statement emission (LET, PRINT, INPUT, END, DIM)
  - FOR loop helpers (init, condition, increment)
  - IF condition evaluation
  - Variable and array access
  - Type inference and automatic conversion
  - Arithmetic, comparison, logical, and string operations
- **CFGEmitter** - CFG block and edge emission (cfg_emitter.{h,cpp}, 436 lines)
  - CFG-aware block traversal (emits ALL blocks including UNREACHABLE)
  - Edge-based control flow (FALLTHROUGH, CONDITIONAL, MULTIWAY, RETURN)
  - Proper terminator emission (jmp, jnz, ret)
  - Block ordering and reachability analysis
  - Label management and resolution
  - Loop header and exit block detection
- **QBECodeGeneratorV2** - Main orchestrator (qbe_codegen_v2.{h,cpp}, 352 lines)
  - Program generation (main + functions + SUBs)
  - Global variable and array declarations
  - Function/SUB generation with parameters
  - Runtime declarations
  - Component coordination
  - IL output management

### 🚧 In Progress
- Integration with compiler (`fbc_qbe.cpp`) - NEXT
- `-i` flag for IL output

### 📋 TODO
- Test suite validation (125 tests)
- Documentation updates
- Performance tuning

## Design Principles

1. **CFG-First**: Use explicit edges, not sequential block numbering
2. **Modular**: Each component has a single responsibility
3. **Testable**: Each component can be tested independently
4. **Edge-Aware**: Handle all edge types (FALLTHROUGH, CONDITIONAL, MULTIWAY, RETURN)
5. **Complete Emission**: Emit ALL blocks, including UNREACHABLE (for GOSUB/ON GOTO)

## Key Differences from Old Generator

| Old Generator | New Generator (V2) |
|---------------|-------------------|
| Sequential block numbers | CFG edge-based traversal |
| Implicit fallthrough | Explicit edge handling |
| Assumes block N+1 follows block N | Uses out-edges for control flow |
| Fragile to CFG changes | Robust, works with any CFG structure |
| Monolithic | Modular (7 components) |

## Usage (When Complete)

```bash
# Compile and emit IL
./fbc_qbe -i test.bas > test.ssa

# Compile, assemble, and link
./fbc_qbe test.bas -o test
./test
```

## Testing Strategy

1. **Unit Tests**: Test each component in isolation
2. **IL Review**: Use `-i` flag to inspect generated IL
3. **Integration Tests**: Run full test suite (125 tests)
4. **Progressive Validation**: Start with simple programs, add complexity

## Documentation

- Design plan: `docs/CODEGEN_V2_DESIGN_PLAN.md`
- Action plan: `docs/CODEGEN_V2_ACTION_PLAN.md`
- Quick start: `docs/CODEGEN_V2_START_HERE.md`
- Status: `docs/STATUS_AND_NEXT_STEPS.md`

## Timeline

- **Week 1**: ✅ COMPLETE - All core components implemented
  - QBEBuilder ✅ (273 lines)
  - TypeManager ✅ (293 lines)
  - SymbolMapper ✅ (342 lines)
  - RuntimeLibrary ✅ (240 lines)
  - ASTEmitter ✅ (639 lines)
  - CFGEmitter ✅ (436 lines)
  - QBECodeGeneratorV2 ✅ (352 lines)
- **Week 2**: Integration and testing
- **Week 3**: Validation with full test suite

## Progress Log

### 2026-02-01 - All Components Complete! 🎉

**Foundation (Morning)**
- ✅ Created codegen_v2 directory structure
- ✅ Implemented QBEBuilder (273 lines) - Low-level IL emission
  - All QBE instruction types supported
  - Temporary variable management
  - Proper function/block structure
  - Type conversion support
- ✅ Implemented TypeManager (293 lines) - Type system
  - Complete BaseType to QBE type mapping
  - Type promotion and conversion logic
  - Support for all BASIC types (INTEGER, LONG, SINGLE, DOUBLE, STRING, etc.)
- ✅ Implemented SymbolMapper (342 lines) - Name mangling
  - Handles BASIC type suffixes (%, $, #, !, &)
  - QBE reserved word escaping
  - Scope-aware naming (global vs. local)
  - Unique label generation

**Core Components (Afternoon)**
- ✅ Implemented RuntimeLibrary (240 lines) - Runtime function wrappers
  - All print operations (int, float, double, string)
  - Complete string operations (concat, len, chr$, mid$, etc.)
  - Array operations with bounds checking
  - Math functions (abs, sqr, trig, rnd, timer)
  - Input operations for all types
- ✅ Implemented ASTEmitter (639 lines) - Statement/expression emission
  - All expression types (literals, variables, binary/unary ops, arrays, calls)
  - Core statements (LET, PRINT, INPUT, END, DIM)
  - FOR loop support (init, condition, increment)
  - Type inference and automatic conversion
  - Arithmetic, comparison, logical, and string operations
- ✅ Implemented CFGEmitter (436 lines) - **CFG-v2 aware control flow**
  - Edge-based block traversal (FALLTHROUGH, CONDITIONAL, MULTIWAY, RETURN)
  - Emits ALL blocks including UNREACHABLE (critical for GOSUB/ON GOTO)
  - Proper terminators (jmp, jnz, ret)
  - Reachability analysis
  - Label management
- ✅ Implemented QBECodeGeneratorV2 (352 lines) - Main orchestrator
  - Program generation (main + functions + SUBs)
  - Global variable and array declarations
  - Component coordination
  - IL output management

**Total Lines of Code**: ~2,575 lines across 7 components

**Next Step**: Integrate with compiler and run the 125-test suite!

## Contributing

When adding new features:
1. Follow the modular architecture
2. Add comments for complex logic
3. Test with `-i` flag to verify IL
4. Update this README with status

---

**Status**: 🚧 Active Development  
**Started**: 2026-02-01  
**Target Completion**: 2026-02-22