# Code Generator V2 Integration Guide

## Overview

This guide explains how to integrate the new CFG-v2-aware code generator into the FasterBASIC compiler.

## Architecture Summary

The new code generator consists of 7 modular components:

```
QBECodeGeneratorV2 (orchestrator)
├── QBEBuilder (low-level IL emission)
├── TypeManager (type mapping)
├── SymbolMapper (name mangling)
├── RuntimeLibrary (runtime calls)
├── ASTEmitter (statements/expressions)
└── CFGEmitter (CFG-aware control flow)
```

**Total**: ~2,575 lines of code across 14 files

## Key Differences from Old Generator

| Feature | Old Generator | New Generator (V2) |
|---------|--------------|-------------------|
| Block traversal | Sequential (assumes N+1 after N) | Edge-based (uses CFG edges) |
| Control flow | Implicit fallthrough | Explicit edge types |
| Unreachable blocks | Skipped | Emitted (for GOSUB/ON GOTO) |
| Architecture | Monolithic | Modular (7 components) |
| CFG compatibility | Breaks with CFG v2 | Built for CFG v2 |

## Integration Steps

### Step 1: Include Headers

In your main compiler file (e.g., `fbc_qbe.cpp`), add:

```cpp
#include "codegen_v2/qbe_codegen_v2.h"
```

### Step 2: Create Generator Instance

After semantic analysis completes:

```cpp
// After semantic analysis
fbc::QBECodeGeneratorV2 codegen(semanticAnalyzer);

// Optional: enable verbose mode for debugging
codegen.setVerbose(true);
```

### Step 3: Generate IL

Replace the old generator call with the new one:

```cpp
// OLD (remove):
// #include "fasterbasic_qbe_codegen.h"
// std::string qbeIL = generateQBECode(program.get(), cfg.get(), ...);

// NEW (use this):
#include "codegen_v2/qbe_codegen_v2.h"
fbc::QBECodeGeneratorV2 codegen(semanticAnalyzer);
std::string qbeIL = codegen.generateProgram(program.get(), cfg.get());

// Output IL if --emit-qbe flag is set
if (options.emitQBE) {
    // Write to .qbe file (existing behavior)
    std::ofstream qbeOut(qbeFile);
    qbeOut << qbeIL;
    qbeOut.close();
    
    if (options.emitQBEOnly) {
        // Just emit and exit (existing --emit-qbe behavior)
        return 0;
    }
}
```

### Step 4: The Compiler Pipeline

The FasterBASIC compiler already has an integrated pipeline:

```
Source (.bas) → Lexer → Parser → Semantic → CFG → CodeGen → QBE IL (.qbe)
                                                                    ↓
                                                            QBE (integrated)
                                                                    ↓
                                                            Assembly (.s)
                                                                    ↓
                                                            Assembler (as)
                                                                    ↓
                                                            Object (.o)
                                                                    ↓
                                                            Linker (ld/gcc)
                                                                    ↓
                                                            Executable
```

**No external QBE calls needed** - the compiler handles the full pipeline internally.

The existing flags are:
```bash
./fbc_qbe program.bas              # Full compile to executable
./fbc_qbe --emit-qbe program.bas   # Stop after generating .qbe file
./fbc_qbe --emit-asm program.bas   # Stop after generating .s file
./fbc_qbe -c program.bas           # Compile to .o only
./fbc_qbe --run program.bas        # Compile and run immediately
```

### Step 5: Update Build System

Add codegen_v2 files to your Makefile or build script:

```makefile
CODEGEN_V2_SOURCES = \
    src/codegen_v2/qbe_builder.cpp \
    src/codegen_v2/type_manager.cpp \
    src/codegen_v2/symbol_mapper.cpp \
    src/codegen_v2/runtime_library.cpp \
    src/codegen_v2/ast_emitter.cpp \
    src/codegen_v2/cfg_emitter.cpp \
    src/codegen_v2/qbe_codegen_v2.cpp

OBJECTS += $(CODEGEN_V2_SOURCES:.cpp=.o)
```

## Example Integration (Complete)

Here's a complete example showing integration in the compiler:

```cpp
#include "fasterbasic_lexer.h"
#include "fasterbasic_parser.h"
#include "fasterbasic_semantic.h"
#include "fasterbasic_cfg.h"
#include "codegen_v2/qbe_codegen_v2.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    // Parse command line
    std::string inputFile = argv[1];
    bool emitIL = false;
    
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "-i") {
            emitIL = true;
        }
    }
    
    // Read source file
    std::ifstream input(inputFile);
    if (!input.is_open()) {
        std::cerr << "Error: cannot open " << inputFile << std::endl;
        return 1;
    }
    
    // Lex
    FasterBASIC::Lexer lexer(input);
    auto tokens = lexer.tokenize();
    
    // Parse
    FasterBASIC::Parser parser(tokens);
    auto program = parser.parseProgram();
    
    if (!program) {
        std::cerr << "Parse error" << std::endl;
        return 1;
    }
    
    // Semantic analysis
    FasterBASIC::SemanticAnalyzer semantic;
    if (!semantic.analyze(program.get())) {
        std::cerr << "Semantic error" << std::endl;
        return 1;
    }
    
    // Build CFG
    FasterBASIC::CFGBuilder cfgBuilder;
    auto cfg = cfgBuilder.buildProgramCFG(program.get());
    
    if (!cfg) {
        std::cerr << "CFG build error" << std::endl;
        return 1;
    }
    
    // Generate code
    fbc::QBECodeGeneratorV2 codegen(semantic);
    codegen.setVerbose(emitIL);  // Verbose when emitting IL
    
    std::string qbeIL = codegen.generateProgram(program.get(), cfg.get());
    
    if (emitIL) {
        // Just emit IL to stdout
        std::cout << qbeIL;
        return 0;
    }
    
    // Write IL to temporary file
    std::string ssaFile = inputFile + ".ssa";
    std::ofstream ssaOut(ssaFile);
    ssaOut << qbeIL;
    ssaOut.close();
    
    // The compiler handles the rest of the pipeline internally
    // QBE backend, assembler, and linker are all integrated
    // Just return success - the compiler does the rest
    
    std::cout << "Compilation successful!" << std::endl;
    return 0;
}
```

## Testing Strategy

### Phase 1: Smoke Test (Simple Programs)

Start with the simplest programs:

```bash
# Test 1: Hello World
./fbc_qbe --emit-qbe test_hello.bas
cat test_hello.qbe  # Verify IL looks reasonable

# Test 2: Simple arithmetic
./fbc_qbe --emit-qbe test_simple.bas

# Test 3: Variables and LET
./fbc_qbe test_hello.bas  # Full compile to executable
./test_hello              # Run it!
```

### Phase 2: Control Flow

Test control flow structures:

```bash
# IF/THEN/ELSE
./fbc_qbe --emit-qbe test_if.bas

# FOR loops
./fbc_qbe test_for.bas && ./test_for

# WHILE loops
./fbc_qbe test_while.bas && ./test_while

# GOTO and labels
./fbc_qbe test_goto.bas && ./test_goto
```

### Phase 3: Full Test Suite

Run the entire test suite:

```bash
cd tests
./run_all_tests.sh

# Or with the compiler:
for test in *.bas; do
    echo "Testing $test..."
    ./fbc_qbe "$test"
    if [ $? -eq 0 ]; then
        exe="${test%.bas}"
        ./"$exe" && echo "  ✓ Compiled and ran successfully"
    else
        echo "  ✗ Compilation failed"
    fi
done
```

### Phase 4: Runtime Testing

The compiler handles everything - just compile and run:

```bash
# Compile and run (integrated pipeline)
./fbc_qbe test.bas
./test

# Or compile and run immediately
./fbc_qbe --run test.bas

# Debug: see the IL
./fbc_qbe --emit-qbe test.bas
cat test.qbe
```

## Expected IL Structure

A generated .qbe file should have this structure:

```qbe
# =======================================================
#   QBE IL Generated by FasterBASIC Compiler
#   Code Generator: V2 (CFG-aware)
# =======================================================

# === Runtime Library Declarations ===
# Runtime functions are linked from runtime_c library

# === Global Variables ===
data $var_x_int = { w 0 }
data $var_name_str = { l 0 }

# === Main Program ===

export function w $main() {
@block_0
    # Block 0
    %t.0 =w add 10, 20
    %t.1 =w mul %t.0, 2
    call $fb_print_int(w %t.1)
    call $fb_print_newline()
    ret 0
}
```

This .qbe file is then processed by the integrated QBE backend (part of the compiler), which generates assembly, which is then assembled and linked - all automatically.

## Troubleshooting

### Issue: "ERROR: null expression" comments in IL

**Cause**: AST contains null expression pointers

**Fix**: Check parser output, ensure all expressions are properly constructed

### Issue: "WARNING: conditional without IF statement"

**Cause**: Block has CONDITIONAL edges but no IF statement

**Fix**: Check CFG builder - condition should be in block's statement list

### Issue: Segmentation fault during code generation

**Cause**: Null pointer in AST, CFG, or semantic analyzer

**Fix**: Add null checks, run with debugger:
```bash
gdb ./fbc_qbe
run -i test.bas
bt  # backtrace
```

### Issue: "ERROR: array not found"

**Cause**: Array symbol not in semantic analyzer's symbol table

**Fix**: Ensure DIM statement is processed during semantic analysis

### Issue: Compiler fails during QBE backend phase

**Cause**: Invalid QBE IL generated (wrong type, missing label, etc.)

**Fix**: Use --emit-qbe to inspect the IL, then review it:
```bash
./fbc_qbe --emit-qbe test.bas
cat test.qbe  # Check for issues
```
- Check all labels are defined before use
- Verify type consistency (w, l, s, d)
- Ensure all blocks have terminators (jmp, jnz, ret)

## Validation Checklist

Before declaring integration complete:

- [ ] Compiler builds without errors
- [ ] `--emit-qbe` flag generates .qbe files
- [ ] Simple programs compile to executables
- [ ] IF/THEN/ELSE generates correct branches
- [ ] FOR loops generate init/condition/increment
- [ ] WHILE loops generate correct control flow
- [ ] GOTO generates unconditional jumps
- [ ] PRINT statements emit runtime calls
- [ ] Variables are properly declared (global vs. local)
- [ ] Type conversions are automatic
- [ ] String operations call runtime functions
- [ ] UNREACHABLE blocks are still emitted
- [ ] All 125 tests compile without crashes
- [ ] Integrated QBE backend processes IL correctly
- [ ] Runtime execution produces correct results

## Performance Notes

The new generator is designed for correctness, not speed. Performance optimizations can be added later:

- **String interning**: Cache string constants
- **Dead code elimination**: Skip truly unused blocks (not UNREACHABLE ones)
- **Register allocation**: Use local temporaries when possible
- **Peephole optimization**: Eliminate redundant conversions

For now, focus on correctness and compatibility with CFG v2.

## Support and Documentation

- **Design docs**: `docs/CODEGEN_V2_DESIGN_PLAN.md`
- **Action plan**: `docs/CODEGEN_V2_ACTION_PLAN.md`
- **Start guide**: `docs/CODEGEN_V2_START_HERE.md`
- **This directory**: `src/codegen_v2/README.md`

## Next Steps

After integration:

1. Run the full test suite and document results
2. Fix any issues found in testing
3. Add missing statement/expression support as needed
4. Optimize frequently-used patterns
5. Document IL patterns for common BASIC constructs
6. Add runtime library implementations (if not already done)

Good luck! 🚀