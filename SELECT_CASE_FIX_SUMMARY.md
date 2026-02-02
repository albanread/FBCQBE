# SELECT CASE Control Flow Fix - Summary

**Date:** February 2, 2026  
**Commit:** be8d7f0  
**Status:** ✅ COMPLETE - All tests passing

---

## Problem Statement

SELECT CASE statements were compiling and linking successfully after the string pool fix (commit 10693c4), but runtime behavior was incorrect:

- All CASE clauses were falling through or jumping to CASE ELSE incorrectly
- The selector expression was never compared against the WHEN values
- Tests like `test_select_case_double.bas` would always print "Other" instead of matching cases

### Root Cause

The CFG builder (`cfg_builder_conditional.cpp::buildSelectCase()`) was creating the block structure and edges for SELECT CASE, but the WHEN check blocks were **empty**:

```
Block 0 (Entry) -> contains nothing related to checking WHEN 0
  -> true edge: When_Body_0
  -> false edge: When_Check_1

Block When_Check_1 -> EMPTY (no comparison logic)
  -> true edge: When_Body_1
  -> false edge: When_Check_2
```

When the CFG emitter processed these blocks, it saw conditional edges but no statements to generate condition code. It would default to `condition = "1"` (always true) or generate invalid branches.

---

## Solution: Synthetic IF Statements in CFG Builder

### Architectural Approach

The correct place to fix this is **in the CFG layer**, not the AST emitter:

1. **CFG Builder** creates synthetic `IfStatement` objects for each WHEN check
2. Each synthetic IF contains the comparison logic (selector == value)
3. The CFG emitter processes these IFs like any other conditional statement
4. No changes needed to the emitter - it already knows how to handle IF statements

### Implementation Details

#### 1. Expression Cloning Helpers

Added `cloneExpression()` to create deep copies of expressions:
- Handles all common expression types (numbers, strings, variables, binary, unary, function calls, arrays)
- Required because we need multiple copies of the selector expression (one per WHEN check)

#### 2. Condition Builders

Three helper functions to create WHEN clause conditions:

**Simple Value Matching:**
```cpp
// CASE 1, 2, 3
// Generates: (selector == 1) OR (selector == 2) OR (selector == 3)
ExpressionPtr createWhenCondition(...) {
    for each value in clause.values:
        create: selector == value
        combine with OR
}
```

**Range Matching:**
```cpp
// CASE 1 TO 10
// Generates: (selector >= 1) AND (selector <= 10)
ExpressionPtr createRangeCheck(...) {
    return: (selector >= start) AND (selector <= end)
}
```

**CASE IS:**
```cpp
// CASE IS < 50
// Generates: (selector < 50)
ExpressionPtr createCaseIsCheck(...) {
    return: selector <op> value
}
```

#### 3. Modified buildSelectCase()

For each WHEN clause:

1. **Create synthetic IF statement:**
   ```cpp
   auto syntheticIf = std::make_unique<IfStatement>();
   syntheticIf->condition = createWhenCondition(stmt, whenClause);
   syntheticIf->isMultiLine = false;
   ```

2. **Add to check block:**
   ```cpp
   addStatementToBlock(previousCaseCheck, syntheticIf.get(), ...);
   previousCaseCheck->statements.back() = syntheticIf.release();
   ```

3. **Wire conditional edges:**
   ```cpp
   addConditionalEdge(checkBlock->id, whenBody->id, "true");
   addConditionalEdge(checkBlock->id, nextCheck->id, "false");
   ```

---

## Generated Code Example

### Input BASIC:
```basic
SELECT CASE x#
    CASE 1.5
        PRINT "1.5"
    CASE 2.5
        PRINT "2.5"
    CASE ELSE
        PRINT "Other"
END SELECT
```

### Generated QBE IL:
```qbe
@block_0
    # First WHEN check (x# == 1.5)
    %t.0 =d loadd %var_x_DOUBLE
    %t.1 =w ceqd %t.0, d_1.5
    jnz %t.1, @block_3, @block_4

@block_3
    # When_Body_0: PRINT "1.5"
    call $basic_print_string_desc(l $str_0)
    jmp @block_2

@block_4
    # Second WHEN check (x# == 2.5)
    %t.3 =d loadd %var_x_DOUBLE
    %t.4 =w ceqd %t.3, d_2.5
    jnz %t.4, @block_5, @block_6

@block_5
    # When_Body_1: PRINT "2.5"
    call $basic_print_string_desc(l $str_1)
    jmp @block_2
```

**Key Points:**
- Each check block now contains real comparison code
- Proper conditional branches (`jnz`) to when-body or next-check
- The emitter didn't need any changes - it processes the synthetic IFs normally

---

## Test Results

All SELECT CASE tests now pass with correct runtime behavior:

### ✅ test_select_case_double.bas
```
Input:  x# = 2.5
Output: 2.5
```

### ✅ test_select_case_comprehensive.bas
Tests all major features:
- Integer and double SELECT
- Range matching (CASE 1 TO 10)
- Multiple values (CASE 2, 4, 6, 8)
- CASE IS with relational operators
- CASE ELSE fallthrough

**All 7 test cases produce correct output.**

### ✅ test_select_mixed_types.bas
Tests type coercion:
- Integer SELECT with integer CASE values
- Double SELECT with integer range
- Type-suffixed variables

**All tests match correctly.**

---

## CFG Structure Verification

Using `-G` flag to trace CFG construction:

```
Block 0 (Entry):
  Statements: 3
    [0] DimStatement
    [1] LetStatement  
    [2] IfStatement    <- Synthetic IF for WHEN 0 check
  Successors: 3 (true), 4 (false)

Block 4 (When_Check_1):
  Statements: 1
    [0] IfStatement    <- Synthetic IF for WHEN 1 check
  Successors: 5 (true), 6 (false)
```

The synthetic IF statements are properly embedded in the CFG blocks.

---

## Why This Approach is Correct

### ✅ Separation of Concerns
- **CFG layer** owns control flow structure and semantics
- **AST layer** owns expression evaluation
- **Emitter layer** translates CFG to target code

### ✅ Consistency
- SELECT CASE now uses the same IF-processing infrastructure as regular IF statements
- No special cases needed in the emitter
- Reuses existing, well-tested condition evaluation code

### ✅ Maintainability
- All SELECT CASE logic is in one place (`cfg_builder_conditional.cpp`)
- Easy to extend (e.g., adding CASE IS BETWEEN)
- Clear separation from other CFG constructs

### ✅ Type Safety
- Expression cloning preserves type information
- Type coercion happens naturally through existing expression evaluation
- No manual type handling needed

---

## Alternative Approaches Considered (and Rejected)

### ❌ Option 1: Generate comparisons in AST emitter
**Problem:** Violates architectural boundaries. The AST emitter should emit whatever statements exist in a block, not invent new control flow.

### ❌ Option 2: Add metadata to CFG edges/blocks
**Problem:** Requires emitter changes to recognize and handle metadata. More invasive and couples the emitter to SELECT CASE specifics.

### ❌ Option 3: Special-case SELECT in CFG emitter
**Problem:** Creates duplicate logic and makes the emitter harder to maintain. Doesn't leverage existing IF infrastructure.

---

## Related Commits

- **10693c4** - Fixed string constant pool collection for SELECT CASE (link-time fix)
- **edb46b9** - FOR suffix normalization (unrelated, earlier work)
- **be8d7f0** - This commit: SELECT CASE control flow fix (runtime behavior)

---

## Future Enhancements

Possible extensions now that the foundation is solid:

1. **CASE IS BETWEEN:** Range with relational operators
   ```basic
   CASE IS BETWEEN 10 AND 20
   ```

2. **CASE expression:** Evaluate expressions, not just literals
   ```basic
   CASE x% + 5
   ```

3. **Optimization:** Detect sequential integer cases and generate jump table
   ```basic
   CASE 1, 2, 3, 4, 5  ' Could use jump table instead of OR chain
   ```

4. **Fall-through support:** QB64-style `CASE 1, 2, 3, 4` as ranges

---

## Conclusion

The SELECT CASE implementation is now **architecturally sound** and **functionally correct**:

- ✅ Compiles without errors
- ✅ Links without missing symbols
- ✅ Executes with correct runtime behavior
- ✅ All test cases pass
- ✅ Clean CFG structure
- ✅ Efficient generated code
- ✅ Maintainable and extensible

The fix demonstrates the value of fixing problems at the correct architectural layer. By creating synthetic IF statements in the CFG builder, we achieved a clean solution that requires no changes to the emitter and naturally handles all SELECT CASE variants.