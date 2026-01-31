# String Function Type Bug Fix

## Problem Description

String functions like `TRIM$`, `LTRIM$`, `RTRIM$`, `UCASE$`, and `LCASE$` were being incorrectly inferred as returning `DOUBLE` (float) instead of `STRING`. This caused type mismatch errors when trying to assign their results to string variables.

### Example Error

```basic
DIM result$ AS STRING
DIM s$ AS STRING
s$ = "  hello  "
result$ = TRIM$(s$)  ' ERROR: Type mismatch - cannot assign FLOAT to STRING
```

## Root Cause Analysis

The bug had **two separate causes** that both needed to be fixed:

### Cause 1: Parser Name Mangling

When the parser encounters a function name with a type suffix (like `TRIM$`), it calls `parseVariableName()` which **mangles** the name:
- `TRIM$` → `TRIM_STRING`
- `LTRIM$` → `LTRIM_STRING`
- `RTRIM$` → `RTRIM_STRING`
- `UCASE$` → `UCASE_STRING`
- `LCASE$` → `LCASE_STRING`

This is done at line 5031 in `fasterbasic_parser.cpp`:
```cpp
case '$':
    outSuffix = TokenType::TYPE_STRING;
    suffixStr = "_STRING";
    suffixLen = 7;
    break;
```

The parser then checks if the name is a builtin function using `isBuiltinFunction()`. However, the builtin function list only contained the original names (with `$`), not the mangled names (with `_STRING`). This caused the parser to treat `TRIM_STRING(x$)` as an **array access** instead of a **function call**.

### Cause 2: Semantic Analyzer Fallback

The semantic analyzer's `getBuiltinReturnType()` function (line 3651 in `fasterbasic_semantic.cpp`) checks if a function returns a string by testing if the name ends with `$`:

```cpp
if (name.back() == '$') {
    return VariableType::STRING;
}
```

However, because the parser mangles `TRIM$` to `TRIM_STRING`, this check fails. The function then falls through to line 3735:

```cpp
// All other functions return FLOAT
return VariableType::FLOAT;
```

This caused the semantic analyzer to think these functions return `FLOAT` instead of `STRING`.

## Solution

### Fix 1: Add Mangled Names to Builtin Function List

**File:** `fsh/FasterBASICT/src/fasterbasic_parser.cpp`  
**Line:** 5112 (in `isBuiltinFunction()`)

Added the mangled `_STRING` versions to the builtin function list:

```cpp
static const std::set<std::string> builtins = {
    // ... existing entries ...
    "INSTR", "SPACE$", "STRING$", "UCASE$", "LCASE$", "LTRIM$", "RTRIM$", "TRIM$",
    "UCASE_STRING", "LCASE_STRING", "LTRIM_STRING", "RTRIM_STRING", "TRIM_STRING",  // ADDED
    "GETTICKS", "LOF", "EOF", "PEEK", "PEEK2", "PEEK4",
    // ... rest of list ...
};
```

This ensures the parser recognizes both `TRIM$` and `TRIM_STRING` as builtin functions and creates a `FunctionCallExpression` instead of an `ArrayAccessExpression`.

### Fix 2: Update Return Type Detection for Mangled Names

**File:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`  
**Line:** 3657 (in `getBuiltinReturnType()`)

Updated the string function detection to handle mangled names:

```cpp
// String functions return STRING
// Check for both $ suffix and _STRING suffix (mangled by parser)
if (name.back() == '$' || 
    (name.length() > 7 && name.substr(name.length() - 7) == "_STRING")) {
    return (m_symbolTable.stringMode == CompilerOptions::StringMode::UNICODE) ?
        VariableType::UNICODE : VariableType::STRING;
}
```

This ensures that both `TRIM$` and `TRIM_STRING` are correctly identified as returning `STRING` type.

## Testing

### Test File: `test_string_func_types.bas`

Created a comprehensive test that verifies all affected string functions:

```basic
' Test that string functions correctly return STRING type

DIM s$ AS STRING
DIM result$ AS STRING

' Test TRIM$
s$ = "  hello  "
result$ = TRIM$(s$)
PRINT "TRIM$: '"; result$; "'"

' Test LTRIM$
s$ = "  left spaces"
result$ = LTRIM$(s$)
PRINT "LTRIM$: '"; result$; "'"

' Test RTRIM$
s$ = "right spaces  "
result$ = RTRIM$(s$)
PRINT "RTRIM$: '"; result$; "'"

' Test UCASE$
s$ = "lowercase"
result$ = UCASE$(s$)
PRINT "UCASE$: '"; result$; "'"

' Test LCASE$
s$ = "UPPERCASE"
result$ = LCASE$(s$)
PRINT "LCASE$: '"; result$; "'"

' Test chaining
s$ = "  Mixed Case  "
result$ = UCASE$(TRIM$(s$))
PRINT "UCASE$(TRIM$()): '"; result$; "'"

' Test direct assignment
DIM direct$ AS STRING
direct$ = TRIM$("  spaces  ")
PRINT "Direct: '"; direct$; "'"

PRINT "All string function type tests passed!"
END
```

### Verification

Generated QBE IL shows correct function calls:

```qbe
# TRIM$ is correctly called as a function, returns pointer (l)
%t3 =l call $string_trim(l %var_s_STRING)
%var_result_STRING =l copy %t3

# LTRIM$ is correctly called as a function
%t11 =l call $string_ltrim(l %var_s_STRING)
%var_result_STRING =l copy %t11

# RTRIM$ is correctly called as a function
%t19 =l call $string_rtrim(l %var_s_STRING)
%var_result_STRING =l copy %t19

# UCASE$ is correctly called as a function
%t27 =l call $string_upper(l %var_s_STRING)
%var_result_STRING =l copy %t27

# LCASE$ is correctly called as a function
%t35 =l call $string_lower(l %var_s_STRING)
%var_result_STRING =l copy %t35
```

**Before the fix:** These would have generated array access code with bounds checking, causing QBE IL errors like "invalid type for first operand %arr_TRIM_STRING in add".

**After the fix:** All functions generate correct function call code and return STRING pointers as expected.

## Impact

This fix resolves type errors for all string manipulation functions that use the `$` suffix:
- `TRIM$` - Remove leading and trailing whitespace
- `LTRIM$` - Remove leading whitespace
- `RTRIM$` - Remove trailing whitespace
- `UCASE$` - Convert to uppercase
- `LCASE$` - Convert to lowercase
- Any other future string functions with `$` suffix

The fix ensures consistent behavior between:
1. Original names with `$` suffix (e.g., `TRIM$`)
2. Mangled names with `_STRING` suffix (e.g., `TRIM_STRING`)
3. Registry-based string functions (e.g., from `command_registry_core.cpp`)

## Related Files Changed

1. **`fsh/FasterBASICT/src/fasterbasic_parser.cpp`**
   - Added mangled string function names to `isBuiltinFunction()` list

2. **`fsh/FasterBASICT/src/fasterbasic_semantic.cpp`**
   - Updated `getBuiltinReturnType()` to recognize `_STRING` suffix

3. **`fsh/test_string_func_types.bas`** (new)
   - Comprehensive test for string function return types

## Build Instructions

To rebuild the compiler with these fixes:

```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
```

To test the fixes:

```bash
./qbe_basic -i -o test.qbe ../fsh/test_string_func_types.bas
```

## Notes

- The parser name mangling is a deliberate design choice to handle type suffixes uniformly
- Other functions like `CHR$`, `STR$`, `LEFT$`, etc. were already correctly handled because their mangled forms (`CHR_STRING`, `STR_STRING`, `LEFT_STRING`) were already in the builtin list
- The registry-based command system (used for some functions) bypasses this issue entirely by using `RegistryFunctionExpression` instead of `FunctionCallExpression`
