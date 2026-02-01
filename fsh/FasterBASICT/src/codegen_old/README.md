# QBE Code Generator - Modular Architecture

This directory contains the modular implementation of the FasterBASIC QBE code generator.

## Overview

The QBE code generator translates FasterBASIC programs from the Control Flow Graph (CFG) representation into QBE Intermediate Language (IL). The generated QBE IL is then compiled to native machine code using the QBE backend.

## Architecture

The code generator is split into focused, maintainable modules:

```
codegen/
├── qbe_codegen_main.cpp        - Main orchestration & block emission
├── qbe_codegen_expressions.cpp - Expression evaluation & emission
├── qbe_codegen_statements.cpp  - Statement emission (PRINT, IF, FOR, etc.)
├── qbe_codegen_runtime.cpp     - Runtime library call wrappers
└── qbe_codegen_helpers.cpp     - Utility functions
```

All modules share a single header: `../fasterbasic_qbe_codegen.h`

## Module Responsibilities

### qbe_codegen_main.cpp

**Purpose:** Main code generation orchestration and control flow

**Key Functions:**
- `generate()` - Entry point, orchestrates full compilation
- `emitHeader()` - Generates QBE IL file header
- `emitDataSection()` - Emits string literals and constants
- `emitMainFunction()` - Generates main() function with CFG-driven blocks
- `emitBlock()` - Emits a single basic block with control flow

**CFG-Driven Approach:**
- Iterates through CFG basic blocks in order
- Emits label for each block (`@block_0`, `@block_1`, etc.)
- Processes all statements within each block
- Uses successor information to emit proper control flow:
  - 0 successors → jump to exit
  - 1 successor → jump (or fall through if sequential)
  - 2 successors → conditional branch (IF/WHILE)
  - 3+ successors → multi-way branch (ON GOTO/SELECT)

### qbe_codegen_expressions.cpp

**Purpose:** Expression evaluation and code emission

**Key Functions:**
- `emitExpression()` - Dispatcher based on expression type
- `emitNumberLiteral()` - Numeric constants
- `emitStringLiteral()` - String constants
- `emitVariableRef()` - Variable references
- `emitBinaryOp()` - Binary operations (+, -, *, /, <, >, etc.)
- `emitUnaryOp()` - Unary operations (-, NOT)
- `emitFunctionCall()` - Built-in function calls
- `emitArrayAccessExpr()` - Array element access

**AST Types Handled:**
- `NumberExpression` - double values
- `StringExpression` - string values
- `VariableExpression` - variable names
- `BinaryExpression` - left op right
- `UnaryExpression` - op expr
- `FunctionCallExpression` - function(args...)
- `ArrayAccessExpression` - array(indices...)

### qbe_codegen_statements.cpp

**Purpose:** Statement emission for all BASIC commands

**Key Functions:**
- `emitStatement()` - Dispatcher based on statement type
- `emitPrint()` - PRINT statement
- `emitInput()` - INPUT statement
- `emitLet()` - LET assignment (variables and arrays)
- `emitIf()` - IF/THEN/ELSE
- `emitFor()` - FOR loop header
- `emitNext()` - NEXT loop footer
- `emitWhile()` - WHILE loop header
- `emitWend()` - WEND loop footer
- `emitGoto()` - GOTO statement
- `emitGosub()` - GOSUB statement (needs return stack)
- `emitReturn()` - RETURN from GOSUB
- `emitDim()` - DIM array declaration
- `emitEnd()` - END program
- `emitRem()` - REM comment
- `emitExit()` - EXIT FOR/WHILE/DO

**Statement Structure Notes:**
- `PrintStatement` has `trailingNewline` (not `suppressNewline`)
- `InputStatement.prompt` is a string (not Expression*)
- `LabelSymbol` has `programLineIndex` (not `blockId`)

### qbe_codegen_runtime.cpp

**Purpose:** Wrappers for runtime library function calls

**Key Functions:**

**I/O Operations:**
- `emitPrintValue()` - Print values by type
- `emitPrintNewline()` - Print newline
- `emitPrintTab()` - Print tab
- `emitInputString()` - Read string from console
- `emitInputInt()` - Read integer
- `emitInputDouble()` - Read double

**String Operations:**
- `emitStringConstant()` - Create/reuse string literal
- `emitStringConcat()` - Concatenate strings
- `emitStringCompare()` - Compare strings
- `emitStringLength()` - Get string length
- `emitStringSubstr()` - Extract substring

**Array Operations:**
- `emitArrayCreate()` - Allocate array with dimensions
- `emitArrayGet()` - Get array element
- `emitArrayStore()` - Set array element

**Type Conversions:**
- `emitIntToString()`, `emitDoubleToString()`
- `emitStringToInt()`, `emitStringToDouble()`
- `emitIntToDouble()`, `emitDoubleToInt()`

**Math Operations:**
- `emitMathFunction()` - Generic math function call
- `emitAbs()`, `emitSqrt()`, `emitSin()`, `emitCos()`, `emitTan()`, `emitPow()`, `emitRnd()`

**File I/O:**
- `emitFileOpen()`, `emitFileClose()`
- `emitFileRead()`, `emitFileWrite()`
- `emitFileEof()`

### qbe_codegen_helpers.cpp

**Purpose:** Utility functions and type system

**Key Functions:**

**Code Emission:**
- `emit()` - Output raw text
- `emitLine()` - Output line with newline
- `emitComment()` - Output comment (if enabled)
- `emitLabel()` - Output label

**Temporary Management:**
- `allocTemp()` - Allocate SSA temporary (%t0, %t1, ...)
- `freeTemp()` - Free temporary (no-op in SSA)

**Label Generation:**
- `makeLabel()` - Create unique label
- `getBlockLabel()` - Get label for CFG block
- `getLineLabel()` - Get label for BASIC line number

**Type System:**
- `getQBEType()` - Map VariableType to QBE type (w/l/d/s)
- `getQBETypeFromSuffix()` - Map BASIC suffix to QBE type
- `getTypeSuffix()` - Extract type suffix from variable name
- `getVariableType()` - Infer variable type

**Variable/Array References:**
- `getVariableRef()` - Get SSA name for variable (%var_X)
- `getArrayRef()` - Get SSA name for array (%arr_X)
- `declareVariable()` - Register variable
- `declareArray()` - Register array

**String Utilities:**
- `escapeString()` - Escape string for QBE data section
- `getRuntimeFunction()` - Map operation to runtime function name

**Constant Evaluation:**
- `isConstantExpression()` - Check if compile-time constant
- `evaluateConstantInt()` - Evaluate integer constant
- `evaluateConstantDouble()` - Evaluate float constant

**Loop Management:**
- `pushLoop()`, `popLoop()`, `getCurrentLoop()`
- Track exit/continue labels for nested loops

**GOSUB Stack:**
- `pushGosubReturn()`, `popGosubReturn()`, `getCurrentGosubReturn()`
- Track return addresses for GOSUB/RETURN

**Type Inference:**
- `inferExpressionType()` - Infer type from expression
- `promoteToType()` - Type promotion/conversion
- `isNumericType()`, `isIntegerType()`, `isFloatingType()`, `isStringType()`

**Utilities:**
- `toUpper()`, `toLower()` - String case conversion

## QBE Type System

### BASIC → QBE Type Mapping

| BASIC Type | Suffix | QBE Type | Size | Description |
|------------|--------|----------|------|-------------|
| INTEGER    | %      | w        | 32   | Word (signed integer) |
| SINGLE     | !      | s        | 32   | Single-precision float |
| DOUBLE     | #      | d        | 64   | Double-precision float |
| STRING     | $      | l        | 64   | Long (pointer) |
| LONG       | &      | l        | 64   | Long (64-bit integer) |

### Variable Naming Convention

All variables and arrays use consistent SSA naming with **type-mangled names**.

**Important:** The semantic analyzer mangles variable names with type suffixes before they reach the code generator. This makes the QBE IL self-documenting with explicit types.

#### Name Mangling Rules (by Semantic Analyzer)

| BASIC Variable | Type Suffix | Mangled Name | QBE Variable |
|----------------|-------------|--------------|--------------|
| `X%`           | % (INTEGER) | `X_INT`      | `%var_X_INT` |
| `Y#`           | # (DOUBLE)  | `Y_DOUBLE`   | `%var_Y_DOUBLE` |
| `S$`           | $ (STRING)  | `S_STRING`   | `%var_S_STRING` |
| `Z!`           | ! (SINGLE)  | `Z_FLOAT`    | `%var_Z_FLOAT` |
| `W`            | (none)      | `W_FLOAT`    | `%var_W_FLOAT` |

**Note:** Variables without a type suffix default to SINGLE (FLOAT) in traditional BASIC.

#### QBE IL Naming Scheme

```
%var_X_INT       - Integer variable X%
%var_Y_DOUBLE    - Double variable Y#
%var_S_STRING    - String variable S$
%arr_A_INT       - Integer array A%()
%t0, %t1         - Temporary values (untyped SSA temps)
%step_I_INT      - FOR loop step value for integer variable I%
%end_I_INT       - FOR loop end value for integer variable I%
```

This convention ensures that:
1. Variable types are immediately visible in QBE IL
2. No name collisions between different types
3. Debugging is easier with self-documenting names
4. QBE IL can be read and understood without the symbol table

## Control Flow

### Basic Block Structure

```qbe
@block_0
    # Block 0 [Lines: 10, 20]
    # Line 10
    %var_X =w copy 5
    # Line 20
    call $basic_print_int(w %var_X)
    jmp @block_1
```

### Loop Structure (FOR)

```qbe
@block_loop_header
    # Initialize loop variable (I% is mangled to I_INT)
    %var_I_INT =w copy 1
    %step_I_INT =w copy 1
    %end_I_INT =w copy 10
    jmp @block_loop_body

@block_loop_body
    # Loop body statements
    call $basic_print_int(w %var_I_INT)
    jmp @block_loop_next

@block_loop_next
    # Increment and check
    %t0 =w add %var_I_INT, %step_I_INT
    %var_I_INT =w copy %t0
    %t1 =w csle %var_I_INT, %end_I_INT
    jnz %t1, @block_loop_body, @block_loop_exit

@block_loop_exit
    # Continue after loop
```

### Conditional (IF/THEN/ELSE)

```qbe
# Evaluate condition
%t0 =w csgt %var_X, 0
jnz %t0, @then_1, @else_1

@then_1
    call $basic_print_string(l $str.0)
    jmp @endif_1

@else_1
    call $basic_print_string(l $str.1)
    jmp @endif_1

@endif_1
    # Continue
```

## Runtime Library Integration

All runtime functions are provided by `libbasic_runtime.a` and linked statically.

### Key Runtime Functions

```c
// Initialization
void basic_init();
void basic_cleanup();

// I/O
void basic_print_int(int32_t value);
void basic_print_double(double value);
void basic_print_string(const char* str);
void basic_print_newline();
int32_t basic_input_int();
double basic_input_double();
char* basic_input_string();

// Strings
char* str_alloc(int32_t size);
char* str_concat(const char* s1, const char* s2);
int32_t str_compare(const char* s1, const char* s2);
int32_t str_length(const char* s);
char* str_substr(const char* s, int32_t start, int32_t len);

// Arrays
void* array_create(int32_t dims, ...);
int32_t array_get(void* arr, ...);
void array_set(void* arr, ..., int32_t value);

// Math
double basic_abs(double x);
double basic_sqrt(double x);
double basic_sin(double x);
double basic_cos(double x);
double basic_tan(double x);
double basic_pow(double base, double exp);
double basic_rnd();
```

## Build Process

The modular structure is built by `build_fbc_qbe.sh`:

```bash
# Compile each module
g++ -c qbe_codegen_main.cpp        → qbe_codegen_main.o
g++ -c qbe_codegen_expressions.cpp → qbe_codegen_expressions.o
g++ -c qbe_codegen_statements.cpp  → qbe_codegen_statements.o
g++ -c qbe_codegen_runtime.cpp     → qbe_codegen_runtime.o
g++ -c qbe_codegen_helpers.cpp     → qbe_codegen_helpers.o

# Link all modules together
g++ fbc_qbe.o lexer.o parser.o semantic.o cfg.o \
    qbe_codegen_*.o ... → fbc_qbe
```

## Benefits of Modular Architecture

1. **Maintainability** - Each file has a single, clear responsibility
2. **Readability** - Smaller files are easier to understand
3. **Parallel Development** - Multiple developers can work on different modules
4. **Testing** - Individual modules can be unit tested
5. **Performance** - Faster compilation (parallel builds, incremental recompilation)
6. **Extensibility** - Easy to add new statements, expressions, or runtime functions

## Example Generated QBE IL

Input BASIC:
```basic
10 FOR I = 1 TO 10
20   PRINT I
30 NEXT I
40 END
```

Generated QBE IL (simplified):
```qbe
export function w $main() {
@start
    call $basic_init()
    %var_I =w copy 0
    jmp @block_0

@block_0
    # Line 10: FOR I = 1 TO 10
    %var_I =w copy 1
    %step_I =w copy 1
    %end_I =w copy 10
    jmp @block_1

@block_1
    # Line 20: PRINT I
    call $basic_print_int(w %var_I)
    call $basic_print_newline()
    jmp @block_2

@block_2
    # Line 30: NEXT I
    %t0 =w add %var_I, %step_I
    %var_I =w copy %t0
    %t1 =w csle %var_I, %end_I
    jnz %t1, @block_1, @block_3

@block_3
    # Line 40: END
    jmp @exit

@exit
    call $basic_cleanup()
    ret 0
}
```

## Future Enhancements

- [ ] Optimize fallthrough to avoid unnecessary jumps
- [ ] Implement GOSUB/RETURN with proper return stack
- [ ] Add DATA/READ/RESTORE support
- [ ] Implement SELECT CASE multi-way branches
- [ ] Add debug info emission (source line mapping)
- [ ] Type inference and automatic promotion
- [ ] Dead code elimination
- [ ] Constant folding
- [ ] Loop unrolling optimization

## References

- [QBE IL Documentation](../../qbe/doc/il.txt)
- [FasterBASIC CFG Structure](../fasterbasic_cfg.h)
- [FasterBASIC AST Types](../fasterbasic_ast.h)
- [Runtime Library](../../runtime_c/)
- [Code Generation Notes](../../CODEGEN_NOTES.md)