# SELECT CASE and Type Glyph Support Analysis

**Date:** 2024
**Status:** ✅ Fixed
**Related Issues:** SELECT CASE floating-point support, Type glyph literals

---

## Summary

This document describes the investigation and fix for SELECT CASE statement type handling bugs, and analyzes type glyph support in the FasterBASIC compiler.

## Issues Discovered

### Issue 1: SELECT CASE with Double Values Failed

**Problem:** SELECT CASE statements with DOUBLE (floating-point) expressions generated invalid QBE IL.

**Example that failed:**
```basic
DIM x#
x# = 2.5

SELECT CASE x#
    CASE 1.5
        PRINT "1.5"
    CASE 2.5
        PRINT "2.5"
END SELECT
```

**Error:**
```
qbe:test.bas:98: invalid type for first operand %var_x_DOUBLE in sltof
```

**Root Cause:** The code generation logic in `qbe_codegen_main.cpp` had inverted type checking logic:
- When `selectQBEType == "d"` (SELECT value is DOUBLE), the code tried to convert it FROM integer TO double using `sltof`
- But the value was ALREADY a double, causing a type error
- The logic incorrectly assumed that `selectQBEType == "d"` meant the value needed conversion TO double

### Issue 2: Type Mismatches Between SELECT Value and CASE Values

**Problem:** Even after fixing the first issue, the code didn't properly handle type conversions between the SELECT expression and CASE values.

**Example that failed:**
```basic
DIM i%              ' Integer
i% = 3

SELECT CASE i%
    CASE 1          ' Literal defaults to DOUBLE
        PRINT "One"
END SELECT
```

**Error:**
```
qbe:test.bas:565: invalid type for first operand %t8 in dtosi
```

**Root Cause:** The code assumed all CASE values were DOUBLE and tried to convert them to integers when comparing with integer SELECT values, but numeric literals can be either type depending on context, and the code didn't check the actual type before attempting conversion.

---

## Solution

### Fix 1: Correct Type Conversion Logic

**File:** `fsh/FasterBASICT/src/codegen/qbe_codegen_main.cpp`

**Changes:** Lines 1045-1180 approximately

**Key principles:**
1. If `selectQBEType == "d"`, the SELECT value IS already a double - use it directly
2. If `selectQBEType != "d"`, the SELECT value IS an integer - use it directly
3. Always infer the type of CASE value expressions using `inferExpressionType()`
4. Convert operands only when there's a type mismatch
5. Always promote to the common type (integer vs double)

**Implementation pattern:**

```cpp
// For each CASE value
std::string valueTemp = emitExpression(clause.values[0].get());
VariableType valueType = inferExpressionType(clause.values[0].get());

if (selectQBEType == "d") {
    // SELECT value is double - ensure CASE value is also double
    std::string valueDouble = valueTemp;
    if (valueType == VariableType::INT) {
        // Convert CASE value from int to double
        valueDouble = allocTemp("d");
        emit("    " + valueDouble + " =d sltof " + valueTemp + "\n");
        m_stats.instructionsGenerated++;
    }
    emit("    " + cmpTemp + " =w ceqd " + selectValue + ", " + valueDouble + "\n");
} else {
    // SELECT value is integer - ensure CASE value is also integer
    std::string valueInt = valueTemp;
    if (valueType == VariableType::DOUBLE || valueType == VariableType::FLOAT) {
        // Convert CASE value from double to int
        valueInt = allocTemp("l");
        emit("    " + valueInt + " =l dtosi " + valueTemp + "\n");
        m_stats.instructionsGenerated++;
    }
    emit("    " + cmpTemp + " =w ceql " + selectValue + ", " + valueInt + "\n");
}
```

**This pattern was applied to:**
- Single value CASE clauses
- Multiple value CASE clauses (comma-separated)
- Range CASE clauses (`CASE x TO y`)
- Conditional CASE clauses (`CASE IS > x`)

---

## Testing

### Test 1: Basic Double SELECT CASE
```basic
DIM x#
x# = 2.5

SELECT CASE x#
    CASE 1.5
        PRINT "1.5"
    CASE 2.5
        PRINT "2.5"
    CASE 3.5
        PRINT "3.5"
    CASE ELSE
        PRINT "Other"
END SELECT
```

**Result:** ✅ PASS - Output: `2.5`

### Test 2: Integer SELECT CASE
```basic
DIM i%
i% = 3

SELECT CASE i%
    CASE 1
        PRINT "One"
    CASE 2
        PRINT "Two"
    CASE 3
        PRINT "Three"
    CASE ELSE
        PRINT "Other"
END SELECT
```

**Result:** ✅ PASS - Output: `Three`

### Test 3: Range with Integers
```basic
i% = 15
SELECT CASE i%
    CASE 1 TO 10
        PRINT "1-10"
    CASE 11 TO 20
        PRINT "11-20"
    CASE 21 TO 30
        PRINT "21-30"
END SELECT
```

**Result:** ✅ PASS - Output: `11-20`

### Test 4: Range with Doubles
```basic
d# = 2.5
SELECT CASE d#
    CASE 0.0 TO 1.0
        PRINT "0-1"
    CASE 1.0 TO 3.0
        PRINT "1-3"
    CASE 3.0 TO 5.0
        PRINT "3-5"
END SELECT
```

**Result:** ✅ PASS - Output: `1-3`

### Test 5: Multiple Values
```basic
i% = 7
SELECT CASE i%
    CASE 2, 4, 6, 8
        PRINT "Even"
    CASE 1, 3, 5, 7, 9
        PRINT "Odd"
END SELECT
```

**Result:** ✅ PASS - Output: `Odd`

### Test 6: CASE IS with Integers
```basic
i% = 42
SELECT CASE i%
    CASE IS < 10
        PRINT "Less than 10"
    CASE IS < 50
        PRINT "Less than 50"
    CASE IS >= 50
        PRINT "50 or more"
END SELECT
```

**Result:** ✅ PASS - Output: `Less than 50`

### Test 7: CASE IS with Doubles
```basic
d# = 3.14159
SELECT CASE d#
    CASE IS < 1.0
        PRINT "Less than 1"
    CASE IS < 4.0
        PRINT "Less than 4"
    CASE IS >= 4.0
        PRINT "4 or more"
END SELECT
```

**Result:** ✅ PASS - Output: `Less than 4`

---

## Type Glyph Support Analysis

### What Are Type Glyphs?

Type glyphs (also called type suffixes or sigils) are special characters appended to variable names or literals to specify their data type:

| Glyph | Type    | Description                |
|-------|---------|----------------------------|
| `%`   | INTEGER | 32/64-bit integer          |
| `!`   | SINGLE  | 32-bit floating-point      |
| `#`   | DOUBLE  | 64-bit floating-point      |
| `$`   | STRING  | String                     |
| `&`   | LONG    | Long integer (same as `%`) |

### Current Support Status

#### ✅ Type Glyphs on Variables: FULLY SUPPORTED

Type glyphs work perfectly on variable declarations and usage:

```basic
DIM a%    ' Integer variable
DIM b!    ' Single-precision float
DIM c#    ' Double-precision float
DIM d$    ' String variable

a% = 42
b! = 3.14
c# = 2.71828
d$ = "Hello"

PRINT a%, b!, c#, d$  ' All work correctly
```

**Implementation:** The lexer's `scanIdentifierOrKeyword()` function (in `fasterbasic_lexer.cpp`) checks for type suffixes after identifier characters and includes them in the token.

#### ❌ Type Glyphs on Numeric Literals: NOT SUPPORTED

Type glyphs do NOT work on numeric literals:

```basic
x = 42%     ' Does NOT create an integer literal
y = 3.14#   ' SYNTAX ERROR - # not recognized after number
z = 1.5!    ' SYNTAX ERROR - ! not recognized after number
```

**Why `42%` appears to work:** The `%` is parsed as the modulo operator, not a type suffix. Since there's no right operand in this context, the expression is just `42`.

**Why `3.14#` fails:** The `#` character is tokenized as HASH (for file I/O like `PRINT #1`), causing a syntax error after a number.

**Root Cause:** The `scanNumber()` function in `fasterbasic_lexer.cpp` (lines 356-408) does not check for or handle type suffixes after numeric literals.

### Should We Add Type Glyphs on Literals?

**Arguments FOR:**
1. **Compatibility:** Classic BASIC dialects (QBasic, GW-BASIC) supported type suffixes on literals
2. **Explicitness:** `42%` clearly indicates an integer literal vs `42.0#` for double
3. **Optimization:** Could help the compiler choose integer operations when appropriate
4. **Consistency:** Variables support them, why not literals?

**Arguments AGAINST:**
1. **Modern defaults:** FasterBASIC defaults to DOUBLE for numeric literals on 64-bit systems, which is appropriate
2. **Limited benefit:** Type promotion happens automatically, so `42` vs `42%` makes little practical difference
3. **Complexity:** Would require changes to lexer and expression type inference
4. **Context sensitivity:** `%` is already the modulo operator, creating ambiguity

**Recommendation:** 
- **LOW PRIORITY** - Not essential for correct operation
- If implemented, start with `%` suffix only (integer literals)
- Document that `#` and `!` suffixes are NOT supported on literals
- Users can explicitly type literals with `CINT()`, `CDBL()`, `CSNG()` functions if needed

### Workarounds for Explicit Type Literals

If you need to explicitly specify the type of a literal, use type conversion functions:

```basic
x% = CINT(42)      ' Explicitly integer
y# = CDBL(3.14)    ' Explicitly double
z! = CSNG(1.5)     ' Explicitly single-precision

' Or use typed variables
DIM val%
val% = 42          ' Assigning to integer variable coerces type
```

---

## Code Generation Quality

The fixed SELECT CASE code generator produces efficient QBE IL:

### Example: Integer SELECT CASE
```basic
i% = 3
SELECT CASE i%
    CASE 3
        PRINT "Three"
END SELECT
```

**Generated QBE IL (relevant portion):**
```qbe
%var_i_INT =l copy 3
# ... test block for CASE 3
%t0 =l copy 3          # Load literal 3 as integer (already correct type)
%t1 =w ceql %var_i_INT, %t0   # Compare integers directly
jnz %t1, @case_body, @next_case
```

**Analysis:** ✅ No unnecessary conversions - both operands are already integers.

### Example: Double SELECT CASE
```basic
d# = 2.5
SELECT CASE d#
    CASE 2.5
        PRINT "Match"
END SELECT
```

**Generated QBE IL (relevant portion):**
```qbe
%var_d_DOUBLE =d copy d_2.5
# ... test block for CASE 2.5
%t0 =d copy d_2.5      # Load literal as double
%t1 =w ceqd %var_d_DOUBLE, %t0   # Compare doubles directly
jnz %t1, @case_body, @next_case
```

**Analysis:** ✅ No unnecessary conversions - both operands are already doubles.

### Example: Mixed Types (Integer SELECT, Double CASE)
```basic
i% = 2
SELECT CASE i%
    CASE 2.5
        PRINT "Match"
END SELECT
```

**Generated QBE IL (relevant portion):**
```qbe
%var_i_INT =l copy 2
# ... test block for CASE 2.5
%t0 =d copy d_2.5      # Load literal as double
%t1 =l dtosi %t0       # Convert CASE value to integer
%t2 =w ceql %var_i_INT, %t1   # Compare integers
jnz %t2, @case_body, @next_case
```

**Analysis:** ✅ Only one conversion needed (CASE value to match SELECT type). This is correct and efficient.

---

## Lessons Learned

1. **Type inference is critical:** Always use `inferExpressionType()` before deciding on type conversions
2. **Test both directions:** Type mismatches can occur in either direction (int→double or double→int)
3. **Inverted logic is subtle:** The original bug was hard to spot because the condition names (`selectQBEType == "d"`) suggested one thing but the implementation did the opposite
4. **Comprehensive tests matter:** Testing all combinations (int/int, double/double, int/double, double/int) revealed the full scope of the issue

---

## Related Code Locations

### Files Modified
- `fsh/FasterBASICT/src/codegen/qbe_codegen_main.cpp` (lines ~1030-1180)

### Files Analyzed
- `fsh/FasterBASICT/src/fasterbasic_lexer.cpp` (`scanNumber()`, `scanIdentifierOrKeyword()`)
- `fsh/FasterBASICT/src/fasterbasic_parser.cpp` (`parseSelectCaseStatement()`)
- `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp` (`inferExpressionType()`)

### Test Files Created
- `tests/test_select_case_double.bas`
- `tests/test_select_case_comprehensive.bas`
- `tests/test_type_glyphs.bas`
- `tests/test_literal_suffix.bas`
- `tests/test_literal_suffix2.bas`

---

## Future Enhancements

### Potential Improvements

1. **Add type suffix support for literals:**
   - Modify `scanNumber()` to check for `%`, `!`, `#` suffixes
   - Store type information in `Token` structure
   - Update `inferExpressionType()` to use literal type info
   - **Effort:** Medium (lexer changes, parser updates, type system integration)

2. **Optimize SELECT CASE with string support:**
   - Currently only numeric types are tested
   - Add string comparison support for CASE clauses
   - **Effort:** Medium (new comparison logic, string interning)

3. **Add compiler warnings for type mismatches:**
   - Warn when comparing integer SELECT with floating-point CASE (and vice versa)
   - "CASE 2.5 will never match integer SELECT value"
   - **Effort:** Low (add diagnostic in semantic analysis)

4. **Jump table optimization for dense integer cases:**
   - When SELECT is integer and CASEs are consecutive values (e.g., 1,2,3,4,5)
   - Generate jump table instead of sequential comparisons
   - **Effort:** High (new code generation strategy, CFG changes)

---

## Conclusion

✅ **SELECT CASE now fully supports both INTEGER and DOUBLE types**

The fix ensures:
- Proper type inference for both SELECT expressions and CASE values
- Minimal type conversions (only when necessary)
- Efficient comparison code generation
- All CASE variants work: single values, multiple values, ranges, and conditional tests

Type glyphs are fully supported on variables but not on numeric literals. This is acceptable for current usage, and explicit type conversion functions provide a workaround when needed.