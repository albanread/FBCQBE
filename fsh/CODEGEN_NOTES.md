# QBE Code Generator - CFG-Driven Implementation Notes

## Overview

The QBE code generator is now CFG-driven, meaning it uses the Control Flow Graph
to emit basic blocks with proper labels and control flow. This is a significant
improvement over the previous AST-walking approach.

## Architecture

### Control Flow Graph (CFG)

The CFG is built in phase 4 of compilation and consists of:

- **BasicBlock**: Contains statements and control flow information
  - `id`: Unique block identifier
  - `label`: Optional descriptive label
  - `statements`: Vector of Statement pointers
  - `successors`: Block IDs this block can jump to
  - `predecessors`: Block IDs that can jump here
  - `lineNumbers`: BASIC line numbers in this block
  - Properties: `isLoopHeader`, `isLoopExit`, `isSubroutine`, `isTerminator`

- **CFGEdge**: Represents control flow between blocks
  - `sourceBlock`, `targetBlock`: Block IDs
  - `type`: FALLTHROUGH, CONDITIONAL, UNCONDITIONAL, RETURN, CALL
  - `label`: Optional edge label (e.g., "true", "false")

- **ControlFlowGraph**: Container for blocks and edges
  - `blocks`: Vector of BasicBlock unique_ptrs
  - `edges`: Vector of CFGEdge
  - `entryBlock`, `exitBlock`: Entry/exit block IDs
  - `lineNumberToBlock`: Maps BASIC line numbers to block IDs
  - Loop tracking maps for FOR, WHILE, REPEAT, DO loops

### Code Generation Strategy

1. **Function Structure**
   ```qbe
   export function w $main() {
   @start
       call $basic_init()
       jmp @block_0
   
   @block_0
       # Block 0 statements...
       jmp @block_1
   
   @block_1
       # Block 1 statements...
       # Conditional branch based on successors
   
   @exit
       call $basic_cleanup()
       ret 0
   }
   ```

2. **Basic Block Emission**
   - Each CFG block gets a unique label: `@block_<id>`
   - Emit all statements in the block sequentially
   - Control flow at end of block:
     - 0 successors → jump to @exit
     - 1 successor → jump to successor (or fall through if next sequential block)
     - 2 successors → conditional branch (IF/WHILE)
     - 3+ successors → multi-way branch (ON GOTO/SELECT CASE)

3. **Loop Structure**
   - FOR loops:
     ```qbe
     @loop_header
         # Initialize loop variable
         # Store step and end values
         jmp @loop_body
     
     @loop_body
         # Loop body statements
         jmp @loop_next
     
     @loop_next
         # Increment loop variable
         # Check condition
         jnz %cond, @loop_body, @loop_exit
     
     @loop_exit
         # Continue after loop
     ```
   
   - WHILE loops:
     ```qbe
     @while_header
         # Evaluate condition
         jnz %cond, @while_body, @while_exit
     
     @while_body
         # Loop body statements
         jmp @while_header
     
     @while_exit
         # Continue after loop
     ```

4. **Conditional Branches**
   - IF statements with inline THEN/ELSE:
     ```qbe
     # Evaluate condition
     %cond =w cne %expr, 0
     jnz %cond, @then_1, @else_1
     
     @then_1
         # THEN statements
         jmp @endif_1
     
     @else_1
         # ELSE statements (if present)
         jmp @endif_1
     
     @endif_1
         # Continue
     ```

## AST Type Mapping

### Expression Types (use getType() → ASTNodeType::EXPR_*)

- `NumberExpression`: Numeric literals (double value)
- `StringExpression`: String literals (std::string value)
- `VariableExpression`: Variable reference (name, typeSuffix)
- `ArrayAccessExpression`: Array element access (name, indices)
- `BinaryExpression`: Binary operations (left, op, right)
  - `op` is TokenType: PLUS, MINUS, MULTIPLY, DIVIDE, EQUAL, NOT_EQUAL, etc.
- `UnaryExpression`: Unary operations (op, expr)
  - `op` is TokenType: MINUS, NOT, etc.
- `FunctionCallExpression`: Function calls (name, arguments, isFN)

### Statement Types (use getType() → ASTNodeType::STMT_*)

- `PrintStatement`: PRINT statement
  - `items`: Vector of PrintItem (expr, semicolon, comma)
  - `fileNumber`: 0 for console, >0 for file
  - `suppressNewline`: If true, don't print newline at end
  
- `InputStatement`: INPUT statement
  - `prompt`: String prompt (not Expression!)
  - `variables`: Vector of variable names (strings)
  - `fileNumber`: 0 for console, >0 for file
  
- `LetStatement`: LET variable = expression
  - `variable`: Variable name (string)
  - `typeSuffix`: TokenType
  - `indices`: For array assignment
  - `value`: Expression to assign
  
- `IfStatement`: IF/THEN/ELSE
  - `condition`: Expression
  - `thenStatements`: Vector of statements
  - `elseStatements`: Vector of statements
  
- `ForStatement`: FOR loop
  - `variable`: Loop variable name (string)
  - `start`, `end`, `step`: Expressions
  
- `NextStatement`: NEXT
  - `variable`: Loop variable name (string, may be empty)
  
- `WhileStatement`: WHILE loop
  - `condition`: Expression
  
- `WendStatement`: WEND (marks end of WHILE)
  
- `GotoStatement`: GOTO
  - `lineNumber`: Target line number (int)
  - `label`: Target label (string)
  - `isLabel`: True if symbolic label
  
- `GosubStatement`: GOSUB
  - `lineNumber`, `label`, `isLabel`: Same as GOTO
  
- `ReturnStatement`: RETURN
  - `returnValue`: Optional expression (nullptr for SUB/GOSUB)
  
- `DimStatement`: DIM arrays
  - `arrays`: Vector of ArrayDeclaration (name, dimensions)
  
- `EndStatement`: END program
  
- `RemStatement`: REM comment
  - `comment`: Comment text (string)
  
- `CallStatement`: CALL subroutine
  
- `ExitStatement`: EXIT FOR/WHILE/DO

## Symbol Table Structure

```cpp
struct SymbolTable {
    std::unordered_map<std::string, VariableSymbol> variables;
    std::unordered_map<std::string, ArraySymbol> arrays;
    std::unordered_map<std::string, FunctionSymbol> functions;
    std::unordered_map<std::string, TypeSymbol> types;
    std::unordered_map<int, LineNumberSymbol> lineNumbers;
    std::unordered_map<std::string, LabelSymbol> labels;
    std::unordered_map<std::string, ConstantSymbol> constants;
    DataSegment dataSegment;
    // ... configuration flags
};

struct VariableSymbol {
    std::string name;
    VariableType type;  // INT, FLOAT, DOUBLE, STRING, UNKNOWN, UNICODE, VOID
    int declaredLine;
    // ... other fields
};

struct ArraySymbol {
    std::string name;
    VariableType elementType;
    std::vector<int> dimensions;
    // ... other fields
};
```

### Accessing Symbols

```cpp
// Iterate variables
for (const auto& [name, varSym] : m_symbols->variables) {
    VariableType type = varSym.type;
    // ...
}

// Iterate arrays
for (const auto& [name, arraySym] : m_symbols->arrays) {
    VariableType elemType = arraySym.elementType;
    // ...
}
```

## Variable Types

```cpp
enum class VariableType {
    INT,        // 32-bit integer (%)
    FLOAT,      // 32-bit float (!)
    DOUBLE,     // 64-bit float (#)
    STRING,     // String ($)
    UNKNOWN,    // Type not yet determined
    UNICODE,    // Unicode string
    VOID        // No return value
};
```

### Type Suffix Mapping

- `%` → INTEGER (w in QBE)
- `!` → FLOAT (s in QBE)
- `#` → DOUBLE (d in QBE)
- `$` → STRING (l in QBE, pointer)
- `&` → LONG (l in QBE, 64-bit int)

### QBE Type Mapping

```cpp
VariableType::INT     → "w"  // word (32-bit)
VariableType::FLOAT   → "s"  // single (32-bit float)
VariableType::DOUBLE  → "d"  // double (64-bit float)
VariableType::STRING  → "l"  // long (pointer, 64-bit)
```

## Runtime Library Functions

All runtime functions are defined in `libbasic_runtime.a` and declared as external
functions in QBE IL.

### I/O Operations
- `basic_init()` - Initialize runtime
- `basic_cleanup()` - Cleanup runtime
- `basic_print_int(w value)` - Print integer
- `basic_print_double(d value)` - Print double
- `basic_print_string(l str)` - Print string (pointer)
- `basic_print_newline()` - Print newline
- `basic_print_tab()` - Print tab
- `basic_input_int()` → w - Read integer
- `basic_input_double()` → d - Read double
- `basic_input_string()` → l - Read string (returns pointer)

### String Operations
- `str_alloc(w size)` → l - Allocate string
- `str_concat(l s1, l s2)` → l - Concatenate strings
- `str_compare(l s1, l s2)` → w - Compare strings
- `str_length(l s)` → w - String length
- `str_substr(l s, w start, w len)` → l - Substring

### Array Operations
- `array_create(w dims, w d1, w d2, ...)` → l - Create array
- `array_get(l arr, w i1, w i2, ...)` → w - Get element
- `array_set(l arr, w i1, w i2, ..., w value)` - Set element

### Conversion Operations
- `int_to_str(w value)` → l - Integer to string
- `double_to_str(d value)` → l - Double to string
- `str_to_int(l str)` → w - String to integer
- `str_to_double(l str)` → d - String to double

## QBE IL Reference

### Basic Instructions

**Arithmetic:**
- `%r =w add %a, %b` - Addition
- `%r =w sub %a, %b` - Subtraction
- `%r =w mul %a, %b` - Multiplication
- `%r =w div %a, %b` - Division (signed)
- `%r =w rem %a, %b` - Remainder (modulo)
- `%r =w neg %a` - Negation

**Comparisons (return w):**
- `%r =w ceq %a, %b` - Equal
- `%r =w cne %a, %b` - Not equal
- `%r =w cslt %a, %b` - Signed less than
- `%r =w csle %a, %b` - Signed less or equal
- `%r =w csgt %a, %b` - Signed greater than
- `%r =w csge %a, %b` - Signed greater or equal

**Logical:**
- `%r =w and %a, %b` - Bitwise AND
- `%r =w or %a, %b` - Bitwise OR
- `%r =w xor %a, %b` - Bitwise XOR

**Control Flow:**
- `jmp @label` - Unconditional jump
- `jnz %cond, @true_label, @false_label` - Conditional jump (non-zero)
- `ret %value` - Return from function

**Function Calls:**
- `%r =w call $function(w %arg1, l %arg2, ...)` - Call function

**Data Movement:**
- `%r =w copy %a` - Copy value
- `%r =w copy 123` - Load immediate

### Type Specifiers

- `w` - Word (32-bit integer)
- `l` - Long (64-bit integer/pointer)
- `s` - Single (32-bit float)
- `d` - Double (64-bit float)
- `b` - Byte (8-bit)
- `h` - Half (16-bit)

### Naming Conventions

- `%name` - Temporary/local variable (SSA)
- `@label` - Label
- `$name` - Global symbol/function
- `123` - Immediate integer
- `d_3.14` - Immediate double
- `s_1.5` - Immediate single

## Implementation Status

### ✅ Implemented
- CFG-driven code generation framework
- Basic block emission with labels
- Function structure (entry/exit)
- Variable declarations
- Control flow based on CFG successors
- Comments and debug info

### 🚧 In Progress
- Expression evaluation (needs AST type fixes)
- Statement emission (needs AST type fixes)
- Runtime function calls

### ❌ Not Yet Implemented
- Loop lowering (FOR/WHILE/DO)
- Array operations
- String operations
- GOSUB/RETURN (needs return stack)
- DATA statements
- File I/O
- User-defined functions
- Type conversions
- Error handling

## Next Steps

1. **Fix AST Type Mismatches** (PRIORITY 1)
   - Replace `BinaryOpExpression` with `BinaryExpression`
   - Replace `UnaryOpExpression` with `UnaryExpression`
   - Replace `LiteralExpression` with `NumberExpression`/`StringExpression`
   - Use `ASTNodeType::STMT_*` instead of `StatementType::*`
   - Fix SymbolTable iteration (use `.variables`, `.arrays` maps)

2. **Complete Expression Emission** (PRIORITY 2)
   - Implement binary operations with TokenType operator mapping
   - Implement unary operations
   - Implement variable load/store
   - Implement array access
   - Implement function calls

3. **Complete Statement Emission** (PRIORITY 3)
   - Fix PRINT statement (use PrintItem structure)
   - Fix INPUT statement (prompt is string, not Expression)
   - Fix LET statement (handle array indices)
   - Implement FOR/NEXT loop lowering
   - Implement WHILE/WEND loop lowering

4. **Control Flow Refinement** (PRIORITY 4)
   - Use CFG successors for all control flow
   - Implement proper conditional branches
   - Optimize fallthrough to sequential blocks
   - Handle multi-way branches (ON GOTO)

5. **Testing** (PRIORITY 5)
   - Test simple programs (PRINT, LET, arithmetic)
   - Test loops (FOR, WHILE)
   - Test conditionals (IF/THEN/ELSE)
   - Test GOTO/GOSUB
   - Test arrays

## Example Output

Input BASIC:
```basic
10 FOR I = 1 TO 10
20   PRINT I
30 NEXT I
40 END
```

Expected QBE IL (conceptual):
```qbe
export function w $main() {
@start
    call $basic_init()
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

## Notes

- QBE is SSA-based, so each temporary gets a unique name
- All control flow must be explicit (jmp/jnz)
- No implicit fallthrough between blocks (except optimized cases)
- String literals are emitted in data section
- All runtime calls are external functions
- Variable storage uses QBE temporaries (not memory loads/stores)