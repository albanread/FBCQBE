# FBCQBE - FasterBASIC QBE Backend

A native-code ahead-of-time (AOT) compiler backend for FasterBASIC using the QBE intermediate language.

## Overview

FBCQBE compiles FasterBASIC programs to native machine code through the QBE (Quick Backend) SSA-based intermediate representation. This provides:

- **Native performance**: Compiled to machine code, not interpreted
- **Strict correctness**: QBE's SSA form enforces proper control flow
- **Platform support**: Works on x86-64, ARM64, and RISC-V via QBE
- **Modern toolchain**: Integrates with standard C compilers and assemblers

## Project Status

### ✅ Working Features

**Control Flow:**
- ✅ IF/THEN/ELSE with proper block structure
- ✅ WHILE/WEND loops with condition checking
- ✅ REPEAT/UNTIL loops
- ✅ DO/LOOP (WHILE/UNTIL) variants
- ✅ FOR/NEXT/STEP loops with full CFG support
  - Nested FOR loops
  - EXIT FOR for early termination
  - Loop index modification (classic BASIC behavior)
  - Expression evaluation in TO clause
- ✅ SELECT CASE multi-way branching
- ✅ GOTO and GOSUB/RETURN

**Functions and Subroutines:**
- ✅ FUNCTION/END FUNCTION with return values
- ✅ SUB/END SUB procedures
- ✅ Recursive functions (Fibonacci, Factorial tested)
- ✅ LOCAL variables with proper scoping
- ✅ SHARED variables (access to globals)
- ✅ RETURN expression for early returns
- ✅ EXIT FUNCTION / EXIT SUB
- 🚧 DEF FN single-line functions (structure complete, type system needs work)

**Data Types:**
- ✅ INTEGER (w/%) - 32-bit signed
- ✅ FLOAT/DOUBLE (d/!) - 64-bit floating point
- ✅ STRING (l/$) - String pointers
- ✅ Type suffix inference (%, #, !, $)

**Statements:**
- ✅ PRINT with formatting
- ✅ LET (assignment)
- ✅ DIM for arrays (structure, runtime integration pending)
- ✅ REM comments
- ✅ END program termination

**Code Generation:**
- ✅ Proper SSA form with explicit temporaries
- ✅ Control Flow Graph (CFG) construction
- ✅ Basic block emission
- ✅ Conditional and unconditional jumps
- ✅ Function calls (user-defined and runtime)
- ✅ Type-appropriate QBE instructions

### 🚧 In Progress

- **Type system**: Full type inference and conversion throughout expressions
- **DEF FN**: Complete implementation with proper typing
- **Arrays**: Full runtime integration for multi-dimensional arrays
- **String operations**: Comprehensive string handling

### 📋 Planned

- **Optimization passes**: Peephole optimization, constant folding
- **Debug info**: Source maps and line number tracking
- **Error handling**: Better error messages with source context
- **Standard library**: Expanded runtime functions (math, string, file I/O)

## Architecture

```
┌─────────────────┐
│  BASIC Source   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│     Lexer       │  Tokenization
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│     Parser      │  AST Construction
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│    Semantic     │  Type checking, symbol resolution
│    Analyzer     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   CFG Builder   │  Control Flow Graph construction
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  QBE CodeGen    │  Generate QBE IL (SSA form)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   QBE Compiler  │  QBE → Assembly
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   Assembler     │  Assembly → Object code
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│     Linker      │  Link with runtime → Executable
└─────────────────┘
```

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 2017+)
- QBE compiler ([https://c9x.me/compile/](https://c9x.me/compile/))
- Standard build tools (make, as, gcc/clang)

### Build the Compiler

```bash
cd fsh
./build_fbc_qbe.sh
```

This produces `fbc_qbe`, the FasterBASIC to QBE compiler.

### Compile a BASIC Program

```bash
# Compile BASIC to QBE IL
./fbc_qbe program.bas

# Compile QBE IL to assembly
qbe a.out > program.s

# Assemble and link
as program.s -o program.o
gcc program.o runtime_stubs.o -o program

# Run
./program
```

## Example Programs

### Hello World
```basic
PRINT "Hello, World!"
END
```

### FOR Loop with Nested Loops
```basic
FOR i% = 1 TO 3
    PRINT "Outer: i% = "; i%
    FOR j% = 1 TO 2
        PRINT "  Inner: j% = "; j%
    NEXT j%
NEXT i%
END
```

### Recursive Function
```basic
FUNCTION Factorial%(N%)
    IF N% <= 1 THEN
        Factorial% = 1
    ELSE
        Factorial% = N% * Factorial%(N% - 1)
    END IF
END FUNCTION

PRINT "5! = "; Factorial%(5)
END
```

### DEF FN (In Progress)
```basic
DEF FN Square(X) = X * X
DEF FN Hypotenuse(A, B) = SQR(A*A + B*B)

PRINT FN Square(5)
PRINT FN Hypotenuse(3, 4)
END
```

## Control Flow Graph (CFG)

The compiler builds an explicit CFG for all code, ensuring correctness:

### FOR Loop CFG Structure
```
┌──────────────┐
│  FOR Init    │  Initialize i, step, end
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  FOR Check   │  if i <= end
└───┬──────┬───┘
    │ true │ false
    ▼      ▼
┌─────┐  ┌──────────┐
│Body │  │After FOR │
└──┬──┘  └──────────┘
   │
   ▼
┌─────────────┐
│    NEXT     │  i = i + step
└──────┬──────┘
       │
       └───────┐
               │
(jump back)    ▼
         ┌──────────────┐
         │  FOR Check   │
         └──────────────┘
```

### Nested FOR Loops
The CFG correctly handles nested loops by creating explicit edges from outer body to inner init:
```
Outer body block → Inner FOR init block → Inner check → Inner body → Inner exit (→ continues outer body)
```

## QBE IL Example

Input:
```basic
FOR i% = 1 TO 5
    PRINT i%
NEXT i%
END
```

Generated QBE:
```qbe
@block_1  # FOR Init
    %var_i_INT =w copy 1
    %step_i_INT =w copy 1
    %end_i_INT =w copy 5

@block_2  # FOR Check
    %t1 =w cslew %var_i_INT, %end_i_INT
    jnz %t1, @block_3, @block_4

@block_3  # FOR Body
    call $basic_print_int(w %var_i_INT)
    call $basic_print_newline()
    %t2 =w add %var_i_INT, %step_i_INT
    %var_i_INT =w copy %t2
    jmp @block_2

@block_4  # After FOR
    jmp @exit
```

## Testing

Comprehensive tests covering:
- Simple and nested FOR loops
- EXIT FOR
- STEP variants (positive, negative, expressions)
- Loop index modification
- Recursive functions
- Multi-function programs
- Control flow edge cases

Run tests:
```bash
cd fsh
for test in ../test_*.bas; do
    echo "Testing $test..."
    ./fbc_qbe "$test" && qbe a.out > a.s && as a.s -o a.o && gcc a.o runtime_stubs.o -o test && ./test
done
```

## Key Implementation Insights

### 1. FOR Loops and CFG
The critical insight: FOR loops need explicit edges in the CFG, not reliance on sequential block IDs. When processing a FOR statement, we create an edge from the current block to the FOR init block, ensuring nested loops work correctly.

### 2. NEXT and Loop Context
NEXT statements are mapped to their loop headers during CFG construction, and this mapping is used during edge generation to create proper back-edges.

### 3. Loop Index Mutability
Unlike some protected implementations, loop indices are fully mutable variables. This matches classic BASIC behavior where `i = limit` forces loop exit - a common idiom in old BASIC code.

### 4. Function Return Values
Functions use a special variable named after the function (e.g., `Factorial%` in a function named `Factorial%`) to store the return value, matching BASIC semantics.

## Runtime

Minimal C runtime (`runtime_stubs.c`) provides:
- `basic_init()` / `basic_cleanup()` - initialization/cleanup
- `basic_print_*()` - output functions for different types
- `basic_input_*()` - input functions
- String and array management (TBD)

## Contributing

This is a research/educational project exploring:
- SSA-based compilation of BASIC
- Control flow graph construction for unstructured languages
- Integration of modern compilation techniques with retro languages

## License

[To be determined]

## Acknowledgments

- **QBE** by Quentin Carbonneaux for the excellent SSA backend
- **FasterBASIC** for the language specification and parser foundation
- Classic BASIC implementations (GW-BASIC, QuickBASIC, BBC BASIC) for inspiration

## References

- QBE IL Documentation: https://c9x.me/compile/doc/il.html
- FasterBASIC: [project link]
- Classic BASIC FOR loop semantics and edge cases