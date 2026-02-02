# SELECT CASE Fix: Before and After Comparison

This document illustrates the problem and solution for SELECT CASE control flow.

---

## The Problem (Before Fix)

### Source Code
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

### Expected Output
```
2.5
```

### Actual Output (Broken)
```
Other
```

### CFG Structure (Broken)

```
Block 0 (Entry)
  Statements:
    [0] DimStatement
    [1] LetStatement (x# = 2.5)
    [2] CaseStatement      <- Just metadata, no comparison!
  Edges:
    -> Block 3 (when_0)
    -> Block 4 (no_match)

Block 3 (When_Body_0)
  Statements:
    [0] PrintStatement("1.5")
  Edges:
    -> Block 2 (Select_Exit)

Block 4 (When_Check_1)
  Statements: EMPTY       <- No comparison code!
  Edges:
    -> Block 5 (when_1)
    -> Block 6 (no_match)
```

**Problem:** Check blocks are empty. The emitter defaults to `condition = "1"` (always true).

### Generated QBE IL (Broken)

```qbe
@block_0
    stored d_2.5, %var_x_DOUBLE
    # No comparison here!
    # Emitter sees conditional edges but no IF statement
    # Defaults to: jmp @block_4  (always goes to next check)

@block_3
    # When_Body_0 (never reached)
    call $basic_print_string_desc(l $str_0)
    jmp @block_2

@block_4
    # When_Check_1 - still no comparison!
    # Eventually falls through to CASE ELSE
```

---

## The Solution (After Fix)

### CFG Structure (Fixed)

```
Block 0 (Entry)
  Statements:
    [0] DimStatement
    [1] LetStatement (x# = 2.5)
    [2] IfStatement        <- Synthetic IF: (x# == 1.5)
      condition: BinaryExpression(==)
        left:  VariableExpression(x#)
        right: NumberExpression(1.5)
  Edges:
    -> Block 3 (true)      <- Match: go to When_Body_0
    -> Block 4 (false)     <- No match: go to When_Check_1

Block 3 (When_Body_0)
  Statements:
    [0] PrintStatement("1.5")
  Edges:
    -> Block 2 (Select_Exit)

Block 4 (When_Check_1)
  Statements:
    [0] IfStatement        <- Synthetic IF: (x# == 2.5)
      condition: BinaryExpression(==)
        left:  VariableExpression(x#)
        right: NumberExpression(2.5)
  Edges:
    -> Block 5 (true)      <- Match: go to When_Body_1
    -> Block 6 (false)     <- No match: go to When_Check_2
```

**Solution:** Each check block now contains a synthetic IF statement with the comparison logic.

### Generated QBE IL (Fixed)

```qbe
@block_0
    %var_x_DOUBLE =l alloc8 8
    stored d_2.5, %var_x_DOUBLE
    
    # First WHEN check: x# == 1.5
    %t.0 =d loadd %var_x_DOUBLE
    %t.1 =w ceqd %t.0, d_1.5
    jnz %t.1, @block_3, @block_4

@block_3
    # When_Body_0: PRINT "1.5"
    %t.2 =l call $string_new_utf8(l $str_0)
    call $basic_print_string_desc(l %t.2)
    call $basic_print_newline()
    jmp @block_2

@block_4
    # Second WHEN check: x# == 2.5
    %t.3 =d loadd %var_x_DOUBLE
    %t.4 =w ceqd %t.3, d_2.5
    jnz %t.4, @block_5, @block_6    <- Branches correctly!

@block_5
    # When_Body_1: PRINT "2.5"
    %t.5 =l call $string_new_utf8(l $str_1)
    call $basic_print_string_desc(l %t.5)
    call $basic_print_newline()
    jmp @block_2
```

### Output (Fixed)
```
2.5
```

✅ **Correct!**

---

## Complex Example: Multiple Values

### Source Code
```basic
DIM i%
i% = 7

SELECT CASE i%
    CASE 2, 4, 6, 8
        PRINT "Even"
    CASE 1, 3, 5, 7, 9
        PRINT "Odd"
END SELECT
```

### Synthetic IF Created (After Fix)

For the second WHEN clause (`CASE 1, 3, 5, 7, 9`):

```
IfStatement:
  condition: BinaryExpression(OR)
    left: BinaryExpression(OR)
      left: BinaryExpression(OR)
        left: BinaryExpression(OR)
          left: BinaryExpression(==)
            left:  VariableExpression(i%)
            right: NumberExpression(1)
          right: BinaryExpression(==)
            left:  VariableExpression(i%)
            right: NumberExpression(3)
        right: BinaryExpression(==)
          left:  VariableExpression(i%)
          right: NumberExpression(5)
      right: BinaryExpression(==)
        left:  VariableExpression(i%)
        right: NumberExpression(7)
    right: BinaryExpression(==)
      left:  VariableExpression(i%)
      right: NumberExpression(9)
```

**In other words:** `(i% == 1) OR (i% == 3) OR (i% == 5) OR (i% == 7) OR (i% == 9)`

### Generated QBE IL

```qbe
# Check: (i% == 1) OR (i% == 3) OR ...
%t.10 =w loadw %var_i_INT
%t.11 =w ceqw %t.10, 1
%t.12 =w loadw %var_i_INT
%t.13 =w ceqw %t.12, 3
%t.14 =w or %t.11, %t.13
%t.15 =w loadw %var_i_INT
%t.16 =w ceqw %t.15, 5
%t.17 =w or %t.14, %t.16
%t.18 =w loadw %var_i_INT
%t.19 =w ceqw %t.18, 7
%t.20 =w or %t.17, %t.19
%t.21 =w loadw %var_i_INT
%t.22 =w ceqw %t.21, 9
%t.23 =w or %t.20, %t.22
jnz %t.23, @when_body_1, @next_check
```

### Output
```
Odd
```

✅ **Correct!**

---

## Range Example

### Source Code
```basic
DIM i%
i% = 15

SELECT CASE i%
    CASE 1 TO 10
        PRINT "1-10"
    CASE 11 TO 20
        PRINT "11-20"
END SELECT
```

### Synthetic IF Created (After Fix)

For `CASE 11 TO 20`:

```
IfStatement:
  condition: BinaryExpression(AND)
    left: BinaryExpression(>=)
      left:  VariableExpression(i%)
      right: NumberExpression(11)
    right: BinaryExpression(<=)
      left:  VariableExpression(i%)
      right: NumberExpression(20)
```

**In other words:** `(i% >= 11) AND (i% <= 20)`

### Generated QBE IL

```qbe
# Check: (i% >= 11) AND (i% <= 20)
%t.5 =w loadw %var_i_INT
%t.6 =w csge %t.5, 11        # i% >= 11
%t.7 =w loadw %var_i_INT
%t.8 =w csle %t.7, 20        # i% <= 20
%t.9 =w and %t.6, %t.8       # Combine with AND
jnz %t.9, @when_body_1, @next_check
```

### Output
```
11-20
```

✅ **Correct!**

---

## CASE IS Example

### Source Code
```basic
DIM i%
i% = 42

SELECT CASE i%
    CASE IS < 10
        PRINT "Less than 10"
    CASE IS < 50
        PRINT "Less than 50"
END SELECT
```

### Synthetic IF Created (After Fix)

For `CASE IS < 50`:

```
IfStatement:
  condition: BinaryExpression(<)
    left:  VariableExpression(i%)
    right: NumberExpression(50)
```

**In other words:** `(i% < 50)`

### Generated QBE IL

```qbe
# Check: i% < 50
%t.3 =w loadw %var_i_INT
%t.4 =w cslt %t.3, 50
jnz %t.4, @when_body_1, @next_check
```

### Output
```
Less than 50
```

✅ **Correct!**

---

## Implementation Summary

### Key Changes in `cfg_builder_conditional.cpp`

**1. Expression Cloning**
```cpp
ExpressionPtr cloneExpression(const Expression* expr) {
    // Deep copy expression trees
    // Needed because we use selector multiple times
}
```

**2. Condition Builders**
```cpp
ExpressionPtr createWhenCondition(
    const CaseStatement& stmt, 
    const WhenClause& clause
) {
    if (clause.isCaseIs) {
        // CASE IS <op> value
        return createCaseIsCheck(...);
    } else if (clause.isRange) {
        // CASE start TO end
        return createRangeCheck(...);
    } else {
        // CASE val1, val2, val3
        // Build: (sel == val1) OR (sel == val2) OR ...
        for (auto& value : clause.values) {
            // Chain with OR
        }
    }
}
```

**3. CFG Building**
```cpp
BasicBlock* CFGBuilder::buildSelectCase(...) {
    for each WHEN clause:
        // Create synthetic IF
        auto syntheticIf = std::make_unique<IfStatement>();
        syntheticIf->condition = createWhenCondition(stmt, clause);
        
        // Add to check block
        addStatementToBlock(checkBlock, syntheticIf.get(), ...);
        checkBlock->statements.back() = syntheticIf.release();
        
        // Wire edges
        addConditionalEdge(checkBlock, whenBody, "true");
        addConditionalEdge(checkBlock, nextCheck, "false");
}
```

---

## Why This Works

1. **Architectural correctness:** Control flow logic belongs in the CFG layer
2. **Code reuse:** Leverages existing IF statement processing
3. **No emitter changes:** The emitter already knows how to handle IF statements
4. **Type safety:** Expression cloning preserves all type information
5. **Extensibility:** Easy to add new CASE variants (e.g., CASE IS BETWEEN)

---

## Test Results

All SELECT CASE tests now pass:

| Test | Status | Description |
|------|--------|-------------|
| test_select_case_double.bas | ✅ | Simple value matching with doubles |
| test_select_case_comprehensive.bas | ✅ | All features: ranges, multiple values, CASE IS |
| test_select_mixed_types.bas | ✅ | Type coercion between int/double |

No regressions in other control flow constructs:
- IF/THEN/ELSE still works ✅
- WHILE loops still work ✅
- FOR loops still work ✅
- GOSUB/RETURN still works ✅

---

## Conclusion

The fix transforms SELECT CASE from broken (always taking CASE ELSE) to fully functional by creating synthetic IF statements in the CFG builder. This approach is architecturally sound, maintainable, and requires no changes to the code emitter.

**Before:** Empty check blocks → no comparisons → incorrect branching  
**After:** Check blocks with synthetic IFs → proper comparisons → correct branching