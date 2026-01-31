# String Function Type Fix - Test Results Summary

**Date:** January 30, 2025  
**Issue:** TRIM$, LTRIM$, RTRIM$, UCASE$, LCASE$ incorrectly inferred as returning DOUBLE instead of STRING  
**Status:** ✅ FIXED

---

## Problem Summary

String manipulation functions with the `$` suffix were being incorrectly typed by the semantic analyzer, causing type mismatch errors:

```basic
DIM result$ AS STRING
result$ = TRIM$("  hello  ")  ' ERROR: Cannot assign FLOAT to STRING
```

### Affected Functions
- `TRIM$` - Remove leading and trailing whitespace
- `LTRIM$` - Remove leading whitespace  
- `RTRIM$` - Remove trailing whitespace
- `UCASE$` - Convert to uppercase
- `LCASE$` - Convert to lowercase

---

## Root Causes Identified

### Issue 1: Parser Name Mangling
The parser converts `TRIM$` → `TRIM_STRING` (strips `$`, adds `_STRING` suffix), but the builtin function list only contained the original `$` forms. This caused the parser to treat these as array accesses instead of function calls.

**Location:** `fsh/FasterBASICT/src/fasterbasic_parser.cpp` line 5112

### Issue 2: Semantic Type Inference  
The semantic analyzer's `getBuiltinReturnType()` only checked for names ending in `$`, missing the mangled `_STRING` forms. These fell through to the default case: `return VariableType::FLOAT`.

**Location:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` line 3657

---

## Fixes Applied

### Fix 1: Updated Parser Builtin List
Added mangled forms to `isBuiltinFunction()`:
```cpp
static const std::set<std::string> builtins = {
    // ... existing entries ...
    "UCASE$", "LCASE$", "LTRIM$", "RTRIM$", "TRIM$",
    "UCASE_STRING", "LCASE_STRING", "LTRIM_STRING", "RTRIM_STRING", "TRIM_STRING",  // ADDED
    // ... rest of list ...
};
```

### Fix 2: Updated Semantic Return Type Detection
Extended string function detection to handle both forms:
```cpp
// Check for both $ suffix and _STRING suffix (mangled by parser)
if (name.back() == '$' || 
    (name.length() > 7 && name.substr(name.length() - 7) == "_STRING")) {
    return VariableType::STRING;
}
```

---

## Verification

### Test File Created
**File:** `fsh/test_string_func_types.bas`

Comprehensive test covering all affected functions:
- Direct assignment: `result$ = TRIM$(s$)`
- Chained calls: `result$ = UCASE$(TRIM$(s$))`
- Inline usage: `PRINT TRIM$("  text  ")`

### QBE IL Generated (Correct)

Before fix:
```qbe
# INCORRECT: Treated as array access with bounds checking
%t2 =l add %arr_TRIM_STRING, 8
%t3 =l loadl %t2
# ... bounds checking code ...
# ERROR: invalid type for first operand %arr_TRIM_STRING in add
```

After fix:
```qbe
# CORRECT: Function calls returning string pointers
%t3 =l call $string_trim(l %var_s_STRING)
%var_result_STRING =l copy %t3

%t11 =l call $string_ltrim(l %var_s_STRING)
%var_result_STRING =l copy %t11

%t19 =l call $string_rtrim(l %var_s_STRING)
%var_result_STRING =l copy %t19

%t27 =l call $string_upper(l %var_s_STRING)
%var_result_STRING =l copy %t27

%t35 =l call $string_lower(l %var_s_STRING)
%var_result_STRING =l copy %t35

# Chained calls work correctly
%t43 =l call $string_trim(l %var_s_STRING)
%t45 =l call $string_upper(l %t43)
```

---

## Test Suite Results

### Full Test Suite Run
```
Tests Run:    53
Tests Passed: 52
Tests Failed: 1
Success Rate: 98.1%
```

### Passing Tests (52/53) ✅
All core functionality tests pass, including:
- ✅ Core Tests (hello, simple, if, goto) - 5/5
- ✅ Arithmetic Tests (integer, double, power, mod, bitwise) - 8/8
- ✅ Comparison & Logical Tests - 4/4
- ✅ Loop Tests (FOR, WHILE, DO...LOOP) - 3/3
- ✅ Control Flow (GOSUB, ON GOTO, ON GOSUB, EXIT) - 4/4
- ✅ Math Functions (intrinsics, conversions) - 2/2
- ✅ Array Tests (1D, 2D, dynamic) - 6/6
- ✅ UDT Tests (basic, nested, arrays) - 6/6
- ✅ String Tests (basic, functions, compare) - 8/8
- ✅ Advanced Features (constants, type handling) - 6/6

### Known Failing Test (1/53) ⚠️
**Test:** `test_string_runtime_functions`  
**Issue:** Integer-to-float conversion (`sltof` expects `l` type, receives `w` type)  
**Status:** Pre-existing issue, **unrelated to string function fix**  
**Note:** This test doesn't actually use TRIM$, UCASE$, or LCASE$ - comment updated to reflect fixes

---

## Files Modified

1. **`fsh/FasterBASICT/src/fasterbasic_parser.cpp`**
   - Line 5112: Added `UCASE_STRING`, `LCASE_STRING`, `LTRIM_STRING`, `RTRIM_STRING`, `TRIM_STRING`

2. **`fsh/FasterBASICT/src/fasterbasic_semantic.cpp`**
   - Line 3657-3659: Extended string function detection for `_STRING` suffix

3. **`fsh/test_string_func_types.bas`** (NEW)
   - Comprehensive test for all fixed string functions

4. **`tests/strings/test_string_runtime_functions.bas`**
   - Line 4: Updated comment noting fixes are complete

5. **`STRING_FUNCTION_TYPE_BUG_FIX.md`** (NEW)
   - Detailed technical documentation of the bug and fix

---

## Impact & Benefits

### ✅ Fixed Issues
- `TRIM$`, `LTRIM$`, `RTRIM$` now correctly return STRING type
- `UCASE$`, `LCASE$` now correctly return STRING type
- Function chaining works: `UCASE$(TRIM$(s$))`
- Direct assignment works: `result$ = TRIM$(input$)`
- Inline usage works: `PRINT "Result: "; TRIM$(s$)`

### ✅ Compatibility
- Both `$` suffix forms (e.g., `TRIM$`) and mangled forms (e.g., `TRIM_STRING`) work correctly
- Registry-based string functions continue to work as before
- No breaking changes to existing working code

### ✅ Code Quality
- Consistent type inference across all string functions
- Proper QBE IL generation (function calls, not array accesses)
- Clean assembly output with correct runtime function calls

---

## Build & Test Instructions

### Rebuild Compiler
```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
```

### Run Full Test Suite
```bash
./test_basic_suite.sh
```

### Test Specific Fix
```bash
cd qbe_basic_integrated
./qbe_basic -i -o test.qbe ../fsh/test_string_func_types.bas
# Verify output shows correct function calls
grep -E "string_trim|string_ltrim|string_rtrim|string_upper|string_lower" test.qbe
```

---

## Conclusion

The string function type inference bug has been **completely resolved**. All affected functions (`TRIM$`, `LTRIM$`, `RTRIM$`, `UCASE$`, `LCASE$`) now correctly return STRING type and generate proper QBE IL function calls.

**Test suite status: 52/53 passing (98.1%)** ✅

The one remaining failure is a pre-existing integer-to-float conversion issue unrelated to this fix.

---

## Related Documentation

- **Technical Details:** `STRING_FUNCTION_TYPE_BUG_FIX.md`
- **Test File:** `fsh/test_string_func_types.bas`
- **Build Instructions:** `qbe_basic_integrated/README.md`
- **String Coercion:** `docs/STRING_COERCION.md`
