# IIF (Immediate IF) Expression Implementation

## Date
January 30, 2025

## Summary
Implemented codegen support for IIF (Immediate IF) expressions, which provide inline conditional evaluation in FasterBASIC.

## Syntax

```basic
result = IIF(condition, trueValue, falseValue)
```

- **condition**: Expression that evaluates to true (non-zero) or false (zero)
- **trueValue**: Expression returned if condition is true
- **falseValue**: Expression returned if condition is false

## Features

### 1. Short-Circuit Evaluation
IIF uses proper short-circuit evaluation - only the selected branch is evaluated:

```basic
x = 10
result = IIF(x > 5, x * 2, x / 0)  ' Division by zero never executed
```

### 2. Type Inference
The result type is automatically inferred from the branches:

```basic
' Integer result
max = IIF(a > b, a, b)

' String result  
message$ = IIF(score >= 60, "Pass", "Fail")

' Mixed types - promotes to wider type
value = IIF(flag, 100, 3.14)  ' Result is DOUBLE
```

### 3. Nested IIF
IIF can be nested for multi-way decisions:

```basic
grade$ = IIF(score >= 90, "A", _
         IIF(score >= 80, "B", _
         IIF(score >= 70, "C", "F")))
```

### 4. Expression Support
IIF can be used anywhere an expression is valid:

```basic
' In assignments
status$ = IIF(code = 200, "OK", "ERROR")

' In PRINT statements
PRINT "Status: "; IIF(connected, "Online", "Offline")

' In calculations
total = base + IIF(discount, base * 0.1, 0)

' With function calls
result$ = IIF(LEN(text$) > 10, UCASE$(text$), LCASE$(text$))
```

## Implementation Details

### Parser (Already Existed)
- **Location**: `fsh/FasterBASICT/src/fasterbasic_parser.cpp:4505-4528`
- Parses `IIF(condition, trueValue, falseValue)` syntax
- Creates `IIFExpression` AST node

### AST Node (Already Existed)
- **Location**: `fsh/FasterBASICT/src/fasterbasic_ast.h:441-465`
- `IIFExpression` class with three members:
  - `condition`: The boolean condition
  - `trueValue`: Expression for true case
  - `falseValue`: Expression for false case

### Codegen (NEW)

#### 1. Expression Dispatcher
- **Location**: `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp:88-90`
- Added case for `ASTNodeType::EXPR_IIF` that calls `emitIIF()`

#### 2. IIF Code Generation
- **Location**: `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp:2147-2228`
- **Method**: `QBECodeGenerator::emitIIF(const IIFExpression* expr)`

**Algorithm:**
1. Infer result type from both branches (handles type promotion)
2. Allocate result temporary with appropriate QBE type
3. Create three labels: `iif_true`, `iif_false`, `iif_end`
4. Evaluate condition expression
5. Convert condition to integer if needed (handles DOUBLE/FLOAT conditions)
6. Convert to boolean using `cnew`
7. Branch using `jnz` instruction
8. **True branch:**
   - Emit true value expression
   - Type-convert if needed
   - Store in result temporary
   - Jump to end
9. **False branch:**
   - Emit false value expression  
   - Type-convert if needed
   - Store in result temporary
   - Fall through to end
10. Return result temporary

#### 3. Type Inference
- **Location**: `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp:916-943`
- Added `ASTNodeType::EXPR_IIF` case to `inferExpressionType()`

**Type Rules:**
- If both branches are same type → return that type
- If either is STRING → return STRING (no auto-conversion)
- Numeric promotion: DOUBLE > FLOAT > INT
- Default to DOUBLE for incompatible types

### Generated QBE Code Example

For: `result = IIF(x > 5, 100, 200)`

```qbe
# IIF expression
%t1 =d copy %var_x
%t2 =d copy d_5.0
%t3 =w cgtd %t1, %t2
%t4 =w dtosi %t3        # Convert condition to int if needed
%t5 =w cnew %t4, 0      # Convert to boolean
jnz %t5, @iif_true_1, @iif_false_2

@iif_true_1
%t6 =d copy d_100.0
%result =d copy %t6
jmp @iif_end_3

@iif_false_2
%t7 =d copy d_200.0
%result =d copy %t7

@iif_end_3
# result now contains the selected value
```

## Type Promotion Examples

```basic
' Both INT → result is INT
result = IIF(flag, 10, 20)

' Both DOUBLE → result is DOUBLE
result = IIF(flag, 1.5, 2.5)

' Mixed INT/DOUBLE → result is DOUBLE
result = IIF(flag, 10, 2.5)

' Both STRING → result is STRING
text$ = IIF(flag, "Yes", "No")

' INT condition with STRING result
text$ = IIF(count > 0, "Found", "Empty")
```

## Files Modified

1. **Codegen - Expression Dispatcher:**
   - `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp`
   - Added case in switch statement (line 88)

2. **Codegen - IIF Implementation:**
   - `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp`
   - Added `emitIIF()` method (lines 2147-2228)

3. **Codegen - Type Inference:**
   - `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp`
   - Added EXPR_IIF case (lines 916-943)

4. **Header File:**
   - `fsh/FasterBASICT/src/fasterbasic_qbe_codegen.h`
   - Added `emitIIF()` declaration (line 269)

## Test Results

**Test File**: `tests/test_iif.bas`

All 10 test groups pass (30+ individual assertions):

✅ Test 1: Basic IIF with Integers  
✅ Test 2: IIF with Strings  
✅ Test 3: IIF with Expressions (variables)  
✅ Test 4: Nested IIF (multi-level conditionals)  
✅ Test 5: IIF with Arithmetic  
✅ Test 6: IIF with Equality Comparisons  
✅ Test 7: IIF in PRINT Statement  
✅ Test 8: IIF with Boolean Values (0/1)  
✅ Test 9: IIF with Function Calls  
✅ Test 10: IIF with AND/OR Conditions  

### Sample Test Output

```
Test 4: Nested IIF
  score = 85
  grade$ = IIF(score >= 90, 'A', IIF(score >= 80, 'B', IIF(score >= 70, 'C', 'F')))
  grade$ = 'B'
  PASS: Nested IIF returned 'B'

Test 9: IIF with Function Calls
  text$ = 'Hello' (length > 3)
  IIF(LEN(text$) > 3, UCASE$(text$), LCASE$(text$)) = 'HELLO'
  PASS: Uppercase when length > 3
```

## Performance Characteristics

- **Short-circuit evaluation**: Only evaluates one branch (important for expensive operations)
- **No function call overhead**: Compiles to inline conditional jumps
- **Type-safe**: Proper type checking and conversion at compile time
- **Efficient**: Minimal overhead compared to IF/THEN/ELSE statements

## Comparison with IF/THEN/ELSE

### Using IF/THEN/ELSE:
```basic
IF score >= 60 THEN
    result$ = "Pass"
ELSE
    result$ = "Fail"
END IF
```

### Using IIF:
```basic
result$ = IIF(score >= 60, "Pass", "Fail")
```

**Advantages of IIF:**
- More concise (one line vs. four lines)
- Can be used in expressions
- Can be nested for complex decisions
- Clearer intent for simple conditions

**When to use IF/THEN/ELSE instead:**
- Multiple statements in each branch
- Complex logic requiring multiple lines
- Need for ELSEIF chains (though nested IIF works)
- Better readability for complex conditions

## Known Limitations

None identified. IIF works correctly with:
- All data types (INT, DOUBLE, STRING)
- Type promotion and conversion
- Nested IIF expressions
- Function calls in any position
- Complex boolean conditions

## Compatibility

IIF is compatible with:
- Visual Basic IIF function
- Other BASIC dialects with conditional expressions
- Follows standard conditional operator semantics

## Future Enhancements

Potential improvements (not required):
1. **Constant folding**: Optimize `IIF(1, x, y)` to just `x` at compile time
2. **Common subexpression elimination**: Detect when branches are identical
3. **Type coercion warnings**: Optional warning for implicit type conversions

## Verification

Run the test to verify:
```bash
./fsh/basic tests/test_iif.bas && ./a.out
```

Expected output: All tests PASS

---

**Status:** ✅ IMPLEMENTED and TESTED  
**Compiler rebuilt:** January 30, 2025  
**All tests passing:** 10/10 test groups, 30+ assertions