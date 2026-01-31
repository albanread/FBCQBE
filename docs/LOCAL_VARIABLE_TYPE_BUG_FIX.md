# LOCAL Variable Type Bug Fix

## Summary

Fixed a critical compiler bug where LOCAL variables in BASIC functions were being assigned with inconsistent QBE types, causing "temporary is assigned with multiple types" errors from the QBE compiler.

## Problem Description

When using LOCAL variables with type suffixes (like `&` for LONG) in BASIC functions, the QBE code generator was:
1. Initializing the variable with one QBE type (e.g., `w` for word/32-bit)
2. Later assigning values with a different QBE type (e.g., `d` for double)

This violated QBE's SSA (Static Single Assignment) requirement that each temporary must have a consistent type throughout its lifetime.

### Example That Failed

```basic
FUNCTION TestFunction&(n AS LONG) AS LONG
    LOCAL x&
    LET x& = n * 2
    RETURN x&
END FUNCTION
```

This would generate invalid QBE IL:
```qbe
%local_x =w copy 0        # Initialized as word (32-bit)
%t1 =l mul %n, 2
%t2 =d sltof %t1
%local_x =d copy %t2      # ERROR: Assigned as double!
```

## Root Causes

### 1. Missing Type Suffix Cases in LOCAL Statement Handler

**File:** `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp`

The `emitLocal()` function's switch statement was missing cases for several type suffixes:
- `TokenType::AMPERSAND` (`&` - LONG suffix)
- `TokenType::PERCENT` (`%` - integer suffix)
- `TokenType::EXCLAMATION` (`!` - float suffix)
- `TokenType::HASH` (`#` - double suffix)
- `TokenType::CARET` (`^` - short suffix)
- `TokenType::AT_SUFFIX` (`@` - byte suffix)

When these were encountered, the default case set `varType = VariableType::FLOAT`, causing type mismatches.

**Fix:** Added all missing type suffix cases to map correctly to `VariableType`.

### 2. Hardcoded QBE Type in Initialization

**File:** `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp`

The initialization code used a hardcoded `=w` (word) for integer types:
```cpp
emit("    " + varRef + " =w copy 0\n");  // Wrong!
```

But `getQBEType(VariableType::INT)` returns `"l"` (64-bit long), not `"w"`.

**Fix:** Changed to use the computed `qbeType` variable:
```cpp
emit("    " + varRef + " =" + qbeType + " copy 0\n");
```

### 3. Number Literals Always Emitted as DOUBLE

**Files:** 
- `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp` (`inferExpressionType`)
- `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp` (`emitNumberLiteral`)

Number literals like `2`, `5`, `10` were:
- **Inferred** as `VariableType::DOUBLE` by `inferExpressionType()`
- **Emitted** as `=d copy d_2.000000` by `emitNumberLiteral()`

This caused expressions like `n& * 2` to be promoted to DOUBLE unnecessarily, then converted back to LONG, creating type mismatches.

**Fix:** 
- Modified `inferExpressionType()` to check if a number is an integer (no decimal point) and return `VariableType::INT` for integer literals
- Modified `emitNumberLiteral()` to emit integer literals as `=l copy N` (64-bit long) instead of doubles

### 4. LOCAL Variable Types Not Tracked

**File:** `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp`

The `getVariableType()` function didn't have access to LOCAL variable type information. It relied on:
1. Symbol table (but LOCAL variables might not be there)
2. Type suffix parsing (but variable names might be sanitized)

This caused the LET statement handler to think it needed type conversions.

**Fix:**
- Added `m_localVariableTypes` map to track types of LOCAL variables
- Populated the map in `emitLocal()` when declaring LOCAL variables
- Check the map first in `getVariableType()` when inside a function
- Clear the map when entering/exiting function contexts

## Changes Made

### Modified Files

1. **fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp**
   - Added missing type suffix cases (AMPERSAND, PERCENT, etc.) in `emitLocal()`
   - Added support for LONG, BYTE, SHORT in AS clause type names
   - Fixed initialization to use computed `qbeType` instead of hardcoded `=w`
   - Store LOCAL variable types in `m_localVariableTypes` map

2. **fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp**
   - Modified `inferExpressionType()` to return INT for integer literals
   - Modified `getVariableType()` to check `m_localVariableTypes` first

3. **fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp**
   - Modified `emitNumberLiteral()` to emit integers as `=l` instead of `=d`

4. **fsh/FasterBASICT/src/fasterbasic_qbe_codegen.h**
   - Added `m_localVariableTypes` member variable

5. **fsh/FasterBASICT/src/codegen/qbe_codegen_main.cpp**
   - Clear `m_localVariableTypes` in `enterFunctionContext()` and `exitFunctionContext()`

### Test Files Added

1. **test_function_local_bug.bas** - Minimal reproduction case with LOCAL LONG variables
2. **test_local_simple.bas** - Simplest possible test with single LOCAL assignment

## Verification

### Before Fix

```
qbe:test_function_local_bug.bas:111: temporary %local_x is assigned with multiple types
```

### After Fix

QBE IL generated successfully with consistent types:
```qbe
%local_x =l copy 0
%t7 =l copy 5
%local_x =l copy %t7
%var_TestFunction =l copy %local_x
```

## Impact

This fix enables:
- ✅ LOCAL variables with type suffixes (`&`, `%`, `@`, `^`) in functions
- ✅ Correct type inference for integer literals in expressions
- ✅ Proper integer arithmetic without unnecessary DOUBLE promotion
- ✅ Function-based implementations (like the Mersenne factors program)

## Related Issues

- Mersenne factors program can now be rewritten using functions instead of GOSUB/RETURN
- Integer-heavy mathematical code will perform better (no float conversions)
- Type system is more consistent with BASIC semantics

## Testing

All existing tests pass, plus new tests verify:
1. Simple LOCAL variable initialization
2. LOCAL variables with arithmetic operations
3. Multiple LOCAL variables of different types in same function
4. LOCAL variables with MOD and other integer operations

---

**Date:** 2024-01-31  
**Author:** AI Assistant with oberon  
**Status:** ✅ Fixed and Verified