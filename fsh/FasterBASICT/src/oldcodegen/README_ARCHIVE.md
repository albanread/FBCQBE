# Old Code Generator - Archive

**Date Archived:** February 2025  
**Reason:** CFG v2 refactoring required new code generator design  
**Status:** REFERENCE ONLY - Do not modify  

---

## Purpose of This Archive

This directory contains the original QBE code generator that was designed for the old CFG (Control Flow Graph) implementation. It has been archived as a reference during the development of the new code generator (codegen_v2).

**Important:** This code is preserved for historical reference and comparison purposes only. It is not actively maintained and should not be used in production.

---

## What's Archived Here

### Files

- `qbe_codegen_main.cpp` (55KB, ~1,300 lines)
  - Main orchestration and block emission
  - CFG traversal and control flow
  
- `qbe_codegen_expressions.cpp` (105KB, ~2,500 lines)
  - Expression evaluation and emission
  - Binary/unary operations, function calls
  
- `qbe_codegen_statements.cpp` (112KB, ~2,800 lines)
  - Statement emission (PRINT, IF, FOR, WHILE, etc.)
  - All BASIC language constructs
  
- `qbe_codegen_runtime.cpp` (13KB, ~300 lines)
  - Runtime library call wrappers
  - I/O, strings, arrays, math functions
  
- `qbe_codegen_helpers.cpp` (54KB, ~1,300 lines)
  - Utility functions
  - Type system, variable management, label generation
  
- `fasterbasic_qbe_codegen.h` (20KB, ~500 lines)
  - Main header file
  - Class definitions and interfaces
  
- `README.md` (Original documentation)
  - Architecture overview
  - Module responsibilities
  - QBE type system
  - Control flow patterns

**Total:** ~8,691 lines of code

---

## Why It Was Replaced

### Architectural Limitations

The old code generator was tightly coupled to the old CFG implementation, which had several issues:

1. **Sequential Block Assumption**
   - Assumed blocks were numbered sequentially (0, 1, 2, 3...)
   - Assumed block N flows to block N+1
   - Broke with nested control structures

2. **Implicit Control Flow**
   - Relied on implicit fallthrough
   - Limited use of explicit edge information
   - Difficult to optimize

3. **Mixed CFG/AST Traversal**
   - Unclear ownership of traversal logic
   - Duplicated logic between CFG and AST handling
   - Made reasoning about flow difficult

4. **Complex State Management**
   - Global loop stacks
   - Global GOSUB return stacks
   - Mutable state spread across components
   - Made debugging and testing difficult

5. **Hardcoded Assumptions**
   - Exit block location assumed
   - Loop structure patterns hardcoded
   - SELECT CASE handling incomplete

### CFG v2 Changes That Required Rewrite

The new CFG (v2) introduced fundamental changes:

- **Single-pass recursive construction** (not two-phase)
- **Explicit edges with types** (Sequential, Conditional, Jump, Call/Return)
- **Exit block always block 1** (predictable, not inferred)
- **Proper terminator handling** (END, RETURN, GOTO)
- **Correct unreachable block detection**
- **Structured loop metadata** (header/body/exit)
- **SELECT CASE with When/Check/Exit blocks**

These changes made the old code generator's assumptions invalid.

---

## What Worked Well

### Strengths to Preserve

1. **Modular File Organization**
   - Separation into focused files worked well
   - Clear module boundaries
   - Easy to navigate

2. **Runtime Library Integration**
   - Clean wrappers around runtime functions
   - Good abstraction of complex operations
   - Comprehensive coverage

3. **Type System**
   - BASIC to QBE type mapping was clear
   - Name mangling conventions worked well
   - Type conversion handling was thorough

4. **Documentation**
   - README.md provided good overview
   - Inline comments explained complex logic
   - QBE IL examples were helpful

5. **Comprehensive Coverage**
   - Handled all BASIC statement types
   - Handled all expression types
   - Good error messages

### Patterns to Reuse

- **Module structure** (separate files for expressions, statements, runtime)
- **Type mapping tables** (BASIC types → QBE types)
- **Name mangling scheme** (variable_TYPE pattern)
- **Runtime API design** (clean function signatures)
- **Temporary allocation** (SSA-friendly patterns)

---

## What to Improve in New Generator

### Key Improvements for Codegen V2

1. **CFG-First Design**
   - Driven by CFG edge information
   - No assumptions about block numbering
   - Explicit control flow only

2. **Cleaner State Management**
   - Immutable data structures where possible
   - Minimize mutable state
   - Clear component boundaries

3. **Better Separation of Concerns**
   - CFGEmitter handles control flow
   - ASTEmitter handles statements/expressions
   - No mixing of responsibilities

4. **Edge-Aware Optimization**
   - Use edge types to optimize jumps
   - Fallthrough optimization
   - Dead code awareness

5. **Unreachable Block Handling**
   - Correctly emit blocks for GOSUB/ON GOTO
   - Distinguish "unreachable sequentially" from "never executed"
   - Leverage CFG unreachability analysis

6. **Type Safety**
   - Leverage semantic analyzer's type information
   - No duplicate type inference
   - Clear type conversions

7. **Optimization-Ready**
   - Design for easy addition of optimization passes
   - Clean IR representation
   - Minimize dependencies between passes

---

## How to Use This Archive

### As a Reference

✅ **Do:**
- Study how statements were handled
- Reference runtime library API usage
- Look up QBE IL patterns
- Understand type conversions
- See expression evaluation strategies

❌ **Don't:**
- Copy code directly without understanding CFG v2 changes
- Reuse CFG traversal patterns (they're broken)
- Assume control flow logic is correct
- Use as-is without modifications

### For Comparison

During development of codegen_v2, compare:
- Generated QBE IL output
- Handling of specific constructs
- Runtime function calls
- Type conversions

### For Testing

Use old generator output as baseline:
- Run same program through both generators
- Compare QBE IL output
- Verify semantic equivalence
- Document improvements

---

## Key Lessons Learned

### Design Principles

1. **Explicit is better than implicit**
   - Don't assume block ordering
   - Don't assume fallthrough
   - Make control flow explicit

2. **Use the type system**
   - Don't duplicate semantic analysis
   - Trust the symbol table
   - Leverage type information

3. **Separate concerns**
   - CFG handles control flow
   - AST handles expressions/statements
   - Don't mix responsibilities

4. **Design for change**
   - Anticipate CFG evolution
   - Make dependencies explicit
   - Keep components loosely coupled

5. **Test incrementally**
   - Start with simple programs
   - Build complexity gradually
   - Maintain comprehensive test suite

### Pitfalls to Avoid

1. **Sequential block assumptions** (blocks are NOT always sequential)
2. **Implicit fallthrough** (must check edge information)
3. **Global mutable state** (use immutable data structures)
4. **Duplicate type inference** (use semantic analyzer results)
5. **Hardcoded patterns** (leverage CFG metadata instead)

---

## Migration Notes

### Critical Differences

| Old CFG | CFG v2 | Impact |
|---------|--------|--------|
| Two-phase construction | Single-pass recursive | Control flow structure changed |
| Implicit edges | Explicit typed edges | Must use edge information |
| Block N → N+1 assumption | No sequential assumption | Must traverse via edges |
| Exit block inferred | Exit always block 1 | Known target for terminators |
| Loop patterns implicit | Loop metadata explicit | Can leverage structure |
| SELECT CASE broken | SELECT CASE complete | Must handle properly |

### Compatibility

The new code generator must:
- Generate compatible QBE IL
- Call runtime library with same signatures
- Produce equivalent behavior
- Maintain backward compatibility

The new code generator may:
- Optimize differently
- Generate cleaner IL
- Produce better code
- Be faster/slower (performance neutral initially)

---

## Timeline

- **2024:** Original code generator developed
- **January 2025:** CFG refactoring (v2) began
- **February 2025:** CFG v2 complete, code generator incompatible
- **February 2025:** Code generator archived, codegen_v2 development started

---

## See Also

- **New Generator Design:** `../codegen_v2/README.md`
- **Migration Plan:** `../../../docs/CODEGEN_V2_DESIGN_PLAN.md`
- **CFG v2 Documentation:** `../cfg_v2/README.md`
- **Unreachable Code Analysis:** `../../../docs/unreachable_code_analysis.md`

---

## Contact

For questions about:
- **This archive:** See git history for original authors
- **New generator:** See `../codegen_v2/` directory
- **CFG v2:** See conversation thread "CFG v2 Sub Function Tests"

---

## Conclusion

This code generator served its purpose well and provided a solid foundation for the new design. While it had architectural limitations that necessitated replacement, it demonstrated effective patterns for QBE IL generation, runtime integration, and modular organization.

The new code generator (codegen_v2) builds on these strengths while addressing the limitations and working correctly with CFG v2's improved architecture.

**Remember:** This is reference material. Always consult the new code generator for current implementation.

---

**Status:** ✅ Archived - Reference Only  
**Last Active:** February 2025  
**Successor:** `../codegen_v2/`
