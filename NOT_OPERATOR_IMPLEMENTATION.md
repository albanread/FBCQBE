# NOT Operator Implementation

## Summary

Successfully implemented and tested the bitwise NOT operator for the FasterBASIC → QBE compiler. The NOT operator now works correctly with integer coercion and follows the same implementation pattern as other integer intrinsics like SGN, ABS, and CINT.

**Test Results: 49/49 tests passing ✅** (increased from 48)

## Changes Made

### 1. Fixed `emitUnaryOp()` in `qbe_codegen_expressions.cpp`

**Location:** Lines 492-526

**Changes:**
- Coerces double/float arguments to 32-bit integer (`w` type) using `dtosi`
- Truncates 64-bit INT (`l`) to 32-bit (`w`) using `copy` instruction
- Performs bitwise NOT using `xor` with -1
- Returns a `w` type result (consistent with SGN, CINT, etc.)

**Code:**
```cpp
case TokenType::NOT: {
    // Bitwise NOT: flip all bits - always returns integer
    // Like SGN/ABS/CINT, coerce argument to integer and return 'w' type
    
    std::string notOperand = operandTemp;
    std::string actualQBEType = getActualQBEType(expr->expr.get());
    
    // Coerce to 32-bit integer if needed
    if (operandType == VariableType::DOUBLE || operandType == VariableType::FLOAT) {
        notOperand = allocTemp("w");
        emit("    " + notOperand + " =w dtosi " + operandTemp + "\n");
        m_stats.instructionsGenerated++;
    } else if (operandType == VariableType::INT) {
        // If it's already INT but stored as 'l' (64-bit), truncate to 'w' (32-bit)
        if (actualQBEType == "l") {
            std::string truncated = allocTemp("w");
            emit("    " + truncated + " =w copy " + notOperand + "\n");
            notOperand = truncated;
            m_stats.instructionsGenerated++;
        }
    } else {
        // String or other type - treat as integer 0
        notOperand = allocTemp("w");
        emit("    " + notOperand + " =w copy 0\n");
        m_stats.instructionsGenerated++;
    }
    
    // Perform bitwise NOT using XOR with -1 on 32-bit value
    std::string notResult = allocTemp("w");
    emit("    " + notResult + " =w xor " + notOperand + ", -1\n");
    m_stats.instructionsGenerated++;
    return notResult;  // Return 'w' type, consistent with other integer intrinsics
}
```

### 2. Added UNARY case to `getActualQBEType()` in `qbe_codegen_helpers.cpp`

**Location:** Lines 210-217

**Changes:**
- NOT expressions now return `'w'` type
- Unary +/- preserve the operand's QBE type

**Code:**
```cpp
// For unary expressions, handle NOT operator
if (nodeType == ASTNodeType::EXPR_UNARY) {
    const UnaryExpression* unaryExpr = static_cast<const UnaryExpression*>(expr);
    if (unaryExpr->op == TokenType::NOT) {
        return "w";  // NOT always returns 'w' (32-bit) like other integer intrinsics
    }
    // For unary + or -, return the type of the operand
    return getActualQBEType(unaryExpr->expr.get());
}
```

### 3. Added UNARY case to `inferExpressionType()` in `qbe_codegen_helpers.cpp`

**Location:** Lines 902-911

**Changes:**
- NOT returns semantic type `VariableType::INT`
- Unary +/- preserve the operand's semantic type
- **This was the critical fix** that prevented incorrect `dtosi` conversion during assignment

**Code:**
```cpp
case ASTNodeType::EXPR_UNARY: {
    const UnaryExpression* unaryExpr = static_cast<const UnaryExpression*>(expr);
    
    // NOT always returns INT
    if (unaryExpr->op == TokenType::NOT) {
        return VariableType::INT;
    }
    
    // Unary + and - preserve the type of the operand
    return inferExpressionType(unaryExpr->expr.get());
}
```

### 4. Created Test File: `tests/arithmetic/test_not_basic.bas`

**Test Coverage:**
- NOT of zero (should be -1)
- NOT of -1 (should be 0)
- NOT of positive numbers (e.g., NOT 5 = -6)
- NOT of negative numbers (e.g., NOT -10 = 9)
- Double NOT (should return original value)
- NOT with literal coercion (e.g., NOT 10)
- NOT in expressions (e.g., (NOT 10) + 1)
- Edge cases (NOT 1, NOT 255)

All 9 test cases pass.

### 5. Updated Test Suite: `test_basic_suite.sh`

**Location:** Lines 216-218

**Changes:**
- Removed skip comment for NOT operator
- Added test_not_basic.bas to the test suite

## Implementation Details

### How NOT Works

The NOT operator follows the same pattern as SGN and ABS:

1. **Argument Coercion:**
   - Arguments are coerced to 32-bit integer (`w` type)
   - DOUBLE/FLOAT → `w` using `dtosi`
   - INT (stored as `l`) → `w` using `copy` (truncation)
   - Other types → treat as 0

2. **Operation:**
   - Bitwise NOT is performed using `xor` with -1
   - This flips all 32 bits in the integer

3. **Result:**
   - Returns `w` type (32-bit integer)
   - When assigned to INT variable (stored as `l`), it's sign-extended using `extsw`

### QBE IL Example

For `LET B% = NOT A%` where `A% = 0`:

```qbe
%t7 =w copy %var_A_INT      # Truncate l to w
%t8 =w xor %t7, -1          # Perform NOT
%t9 =l extsw %t8            # Sign-extend w to l
%var_B_INT =l copy %t9      # Store in variable
```

### Type System Integration

The implementation correctly handles both semantic types and concrete QBE types:

| Context | Semantic Type | QBE Type | Notes |
|---------|--------------|----------|-------|
| NOT argument | Any | Coerced to `w` | dtosi for DOUBLE/FLOAT |
| NOT result | INT | `w` | 32-bit integer |
| INT variable | INT | `l` | 64-bit storage |
| Assignment | - | `extsw` conversion | Sign-extend w→l |

## Test Results

### Individual Test Output

```
=== NOT Operator Tests ===

NOT 0 = -1
PASS: NOT 0 = -1

NOT -1 = 0
PASS: NOT -1 = 0

NOT 5 = -6
PASS: NOT 5 = -6

NOT -10 = 9
PASS: NOT -10 = 9

NOT (NOT 42) = 42
PASS: NOT (NOT 42) = 42

NOT 10 = -11
PASS: NOT 10 (literal) = -11

NOT 1 = -2
PASS: NOT 1 = -2

NOT 255 = -256
PASS: NOT 255 = -256

(NOT 10) + 1 = -10
PASS: (NOT 10) + 1 = -10

=== All NOT Operator Tests PASSED ===
```

### Full Test Suite

```
Total Tests:   49
Passed:        49
Failed:        0
Skipped:       0

✓ ALL TESTS PASSED!
```

## Key Insights

1. **Missing `inferExpressionType()` case was critical:** Without handling EXPR_UNARY, the type inference defaulted to DOUBLE, causing the assignment code to emit incorrect `dtosi` conversion instead of `extsw`.

2. **QBE type consistency matters:** The implementation returns `w` type consistently with other integer intrinsics (SGN, CINT, etc.), which simplifies the type system.

3. **Two-level type tracking:** Both semantic types (VariableType) and concrete QBE types (`w`, `l`, `d`) must be correctly maintained throughout codegen.

4. **QBE's strict typing is beneficial:** QBE caught the type errors immediately, making debugging much easier than it would be with assembly-level errors.

## Files Modified

1. `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp`
2. `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp`
3. `tests/arithmetic/test_not_basic.bas` (new)
4. `test_basic_suite.sh`

## Compilation Status

- ✅ No compilation errors
- ⚠️  Pre-existing warnings (unrelated to NOT implementation)
- ✅ All 49 tests passing

## Next Steps

The NOT operator is now fully functional and integrated. Potential future improvements:

1. **Refactor `emitExpression()` API:** Return both (tempName, QBE type) to avoid type ambiguity
2. **Add more bitwise operator tests:** Test AND, OR, XOR with NOT combinations
3. **Performance optimization:** Consider using QBE's intrinsic NOT if available
4. **Documentation:** Add NOT to operator precedence documentation

## Related Documentation

- See `MATH_OPERATORS_STATUS.md` for overall operator implementation status
- See `SESSION_SUMMARY.md` for development session notes
- See semantic analyzer notes in `fasterbasic_semantic.cpp` for type inference rules