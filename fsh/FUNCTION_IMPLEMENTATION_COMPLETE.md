# Function/Subroutine Implementation - Session Summary

## Overview
Successfully implemented and debugged the multi-function CFG architecture for FasterBASIC's QBE backend. Functions and subroutines now compile to native code through QBE IL.

## What Was Accomplished

### 1. Fixed Compilation Errors ✅

- **Removed duplicate `toString()` declaration** in `ControlFlowGraph` class
- **Fixed all `m_cfg` → `m_currentCFG` references** in `fasterbasic_cfg.cpp`
- **Added const overload for `getFunctionCFG()`** to support const `ProgramCFG` access
- **Fixed `ProgramCFG::getBlockCount()` usage** in `fbc.cpp` and `fbc_qbe.cpp`

### 2. Implemented Multi-Function CFG Architecture ✅

#### CFG Builder (`fasterbasic_cfg.cpp`)
- **Separated FUNCTION/SUB from main CFG**: Function statements no longer appear in main program blocks
- **Created `ProgramCFG` structure**: Contains `mainCFG` + `functionCFGs` map
- **Function metadata extraction**: Captures function name, parameters, parameter types, and return type
- **Separate CFG per function**: Each FUNCTION/SUB gets its own independent `ControlFlowGraph`

#### QBE Code Generator (`qbe_codegen_main.cpp`)
- **Implemented `emitFunction()`**: Generates complete QBE IL for user-defined functions
  - Proper function signature with typed parameters
  - Entry block with return variable initialization
  - Function body blocks with proper control flow
  - Exit block with return statement
- **Parameter handling**: Function parameters use `%param` directly (no `%var_` prefix)
- **Return value handling**: Functions return via function-name variable assignment

### 3. Fixed Function Call Emission ✅

#### Expression Codegen (`qbe_codegen_expressions.cpp`)
- **User-defined function detection**: Checks `ProgramCFG::getFunctionCFG()` before mapping to runtime
- **Direct function calls**: Emits `call $FunctionName(...)` for user functions
- **Runtime fallback**: Still calls `$basic_*` functions for built-ins (SIN, COS, etc.)
- **Return type inference**: Uses function CFG metadata to determine correct QBE return type

### 4. Fixed Type System Issues ✅

#### QBE Type Mappings (`qbe_codegen_helpers.cpp`)
- **FLOAT → `d` (double)**: QBE doesn't support single-precision, use double for all floats
- **INT → `w` (word)**: 32-bit integers
- **STRING → `l` (long)**: 64-bit pointers
- **Changed default function return type**: INT instead of DOUBLE when no type suffix

#### Type Conversions (`qbe_codegen_statements.cpp`)
- **Automatic `swtof` insertion**: Converts int to double when assigning to float variables
- **Type-aware assignment**: Detects type mismatches and inserts conversion instructions

#### Runtime ABI Fix (`runtime_stubs.c`)
- **Fixed `basic_print_float()` signature**: Changed from `float` to `double` parameter
- **Reason**: QBE passes all floating-point as 64-bit doubles (type `d`)
- **Impact**: Fixed the "Result = 0" bug caused by 4-byte vs 8-byte parameter mismatch

### 5. Parameter vs Variable Handling ✅

#### Variable Reference (`qbe_codegen_helpers.cpp`)
- **Parameter detection**: Checks if variable name matches a function parameter
- **Direct parameter reference**: Uses `%param` for parameters (not `%var_param`)
- **Local variable reference**: Uses `%var_name` for regular variables
- **Context-aware**: Only applies parameter logic when `m_inFunction` is true

## Test Results

### ✅ Working Test: Simple Function
```basic
10 FUNCTION Add(X, Y)
20   Add = X + Y
30 END FUNCTION
40 PRINT Add(3, 4)
50 END
```
**Output**: `7` ✅

### ✅ Working Test: Function with Variable Assignment
```basic
10 REM Simple function test
20 FUNCTION Add(X, Y)
30   Add = X + Y
40 END FUNCTION
50 REM
60 LET Result = Add(3, 4)
70 PRINT "3 + 4 = "; Result
80 END
```
**Output**: `3 + 4 = 7.000000` ✅

## Generated QBE IL Example

```qbe
export function w $Add(w %X, w %Y) {
@start
    %var_Add =w copy 0
    # Function body
    jmp @block_0

@block_0
    # Block 0 (Function Entry)
    %t4 =w add %X, %Y
    %var_Add =w copy %t4
    jmp @block_1

@block_1
    # Block 1 (Function Exit)
    jmp @exit

@exit
    # Return from function
    %retval =w copy %var_Add
    ret %retval
}
```

## Known Limitations

### Not Yet Implemented
1. **EXIT FUNCTION / EXIT SUB**: Early return from functions
2. **BYREF parameters**: Pass-by-reference support
3. **Local variables**: DIM/LOCAL inside functions (currently all globals)
4. **Nested functions**: Function definitions inside functions
5. **Recursive functions**: Need stack frame management testing
6. **Function overloading**: Multiple functions with same name but different signatures
7. **Complex control flow in functions**: IF/WHILE/FOR inside functions (needs more testing)

### Potential SSA Issues
- **Variable assignments across blocks**: Current implementation may need PHI nodes for complex control flow
- **Return value tracking**: Using function-name variable across multiple blocks works for simple cases but may break with complex branching

### Type System Limitations
- **Variables default to FLOAT**: Without type suffix, variables are FLOAT/SINGLE, causing conversions
- **No type inference**: Can't infer that `Result = Add(3,4)` should be INT if Add returns INT
- **Manual conversions**: Inserts `swtof` conservatively, may generate unnecessary conversions

## Architecture Benefits

### Clean Separation
- **One CFG per function**: Each function is independent, simplifying analysis and optimization
- **ProgramCFG container**: Clean API for accessing main program and all functions
- **Modular codegen**: Function emission is separate from main program emission

### Extensibility
- **Easy to add features**: EXIT FUNCTION, BYREF, etc. can be added incrementally
- **Optimization ready**: Each function's CFG can be optimized independently
- **Testing friendly**: Can test functions in isolation

## Files Modified

### Core Implementation
- `fasterbasic_cfg.h` - Added ProgramCFG, fixed declarations
- `fasterbasic_cfg.cpp` - Implemented processFunctionStatement/processSubStatement, fixed m_cfg references
- `qbe_codegen_main.cpp` - Implemented emitFunction()
- `qbe_codegen_expressions.cpp` - Added user-function detection and call emission
- `qbe_codegen_statements.cpp` - Added type conversions in LET
- `qbe_codegen_helpers.cpp` - Fixed getQBEType(), added parameter detection in getVariableRef()
- `qbe_codegen_runtime.cpp` - Fixed print_float type
- `fbc_qbe.cpp` - Fixed ProgramCFG access
- `fbc.cpp` - Fixed ProgramCFG access

### Runtime
- `runtime_stubs.c` - Fixed basic_print_float to take double

## Next Steps (Priority Order)

### High Priority
1. **Test complex control flow**: IF/WHILE/FOR inside functions
2. **Implement EXIT FUNCTION/SUB**: Jump to function exit block
3. **Add comprehensive tests**: Nested calls, recursion, edge cases

### Medium Priority
4. **Local variable scoping**: Make DIM inside functions create locals, not globals
5. **BYREF parameters**: Pass pointers for reference parameters
6. **Better type inference**: Reduce unnecessary conversions

### Low Priority
7. **Optimize PHI nodes**: Handle complex SSA cases properly
8. **Function overloading**: Support multiple signatures
9. **Inline functions**: Optimization pass for small functions

## Conclusion

The foundation for user-defined functions is **solid and working**. Simple functions compile, link, and execute correctly. The architecture is clean and extensible. The main remaining work is adding features (EXIT, BYREF, locals) and testing edge cases.

**Status**: ✅ Basic function implementation COMPLETE and TESTED