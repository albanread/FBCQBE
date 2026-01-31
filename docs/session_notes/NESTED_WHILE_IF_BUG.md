# Nested WHILE Inside IF Bug

**Status**: ✅ FIXED  
**Severity**: High  
**Date Discovered**: 2025-01-30  
**Date Fixed**: 2025-01-30

---

## Summary

**FIXED**: This bug has been resolved by implementing recursive CFG processing for nested statements inside IF blocks.

~~WHILE loops nested inside multi-line IF statements do not execute correctly. The inner WHILE loop only runs one iteration instead of looping until its condition becomes false.~~

## Example

```basic
DIM sieve(100) AS INT
DIM i AS INT
DIM j AS INT

i = 2
WHILE i <= 3
    IF sieve(i) = 1 THEN
        j = 4
        WHILE j <= 10    ' <-- Inner WHILE inside IF
            sieve(j) = 0
            j = j + 2
        WEND            ' <-- Only executes once!
    END IF
    i = i + 1
WEND
```

**Expected**: Inner WHILE should run for j=4, 6, 8, 10  
**Actual**: Inner WHILE only runs once with j=4

## Symptoms

1. Inner WHILE loop body executes exactly once
2. Loop counter is incremented correctly (j becomes 6)
3. Loop condition is NOT re-evaluated (doesn't jump back to check j <= 10)
4. Execution continues to WEND and exits the loop

## Root Cause

The CFG (Control Flow Graph) builder does not properly handle nested control-flow structures inside multi-line IF statements.

### Technical Details

1. Multi-line IF statements store their body statements as **children** of the IF node (`thenStatements`, `elseStatements` arrays), not as sequential top-level statements.

2. The CFG builder in `fasterbasic_cfg.cpp` only processes top-level statements in sequence. When it encounters an IF statement with a body, it does NOT recursively process the nested statements.

3. This means nested WHILE/WEND statements inside IF bodies are **invisible** to the CFG builder.

4. The CFG builder never creates loop header blocks or back-edges for these nested WHILE loops.

5. The code generator still emits code for these loops, but without proper CFG structure, the control flow is incorrect.

## Workarounds

### Option 1: Use Single-Line IF with GOTO

```basic
i = 2
WHILE i <= 3
    IF sieve(i) <> 1 THEN GOTO skip_loop
    j = 4
    WHILE j <= 10
        sieve(j) = 0
        j = j + 2
    WEND
skip_loop:
    i = i + 1
WEND
```

### Option 2: Invert the Condition

```basic
i = 2
WHILE i <= 3
    IF sieve(i) <> 1 THEN
        ' Skip the loop
    ELSE
        ' Can't put WHILE here either - same problem!
    END IF
    i = i + 1
WEND
```

(This doesn't actually help - same issue applies to ELSE block)

### Option 3: Restructure Code

Move the inner loop outside the IF:

```basic
i = 2
WHILE i <= 3
    should_process = (sieve(i) = 1)
    IF should_process THEN
        j = 4
    ELSE
        j = 999  ' Skip loop
    END IF
    
    WHILE j <= 10
        sieve(j) = 0
        j = j + 2
    WEND
    
    i = i + 1
WEND
```

### Option 4: Use Nested IFs Instead of WHILE

```basic
i = 2
WHILE i <= 3
    IF sieve(i) = 1 THEN
        j = 4
        IF j <= 10 THEN sieve(j) = 0: j = j + 2
        IF j <= 10 THEN sieve(j) = 0: j = j + 2
        IF j <= 10 THEN sieve(j) = 0: j = j + 2
        ' etc...
    END IF
    i = i + 1
WEND
```

(Only works if iteration count is known at compile time)

## What Works

- **Simple nested WHILE loops** (without IF) work perfectly:
  ```basic
  WHILE i <= 3
      WHILE j <= 3
          PRINT j
          j = j + 1
      WEND
      i = i + 1
  WEND
  ```

- **Single-line IF statements** work fine (they don't nest statements as children)

- **FOR loops** likely have the same issue (untested)

## Fix Implemented

**Solution**: Option B - CFG Builder Recursively Processes Nested Statements

### Implementation Details

Modified `CFGBuilder::processIfStatement()` in `fsh/FasterBASICT/src/fasterbasic_cfg.cpp` to recursively process nested statements inside multi-line IF blocks.

#### Changes Made:

1. **New Helper Method**: Added `CFGBuilder::processNestedStatements()` that:
   - Recursively processes statements nested inside IF's `thenStatements`, `elseIfClauses`, and `elseStatements`
   - Identifies control-flow statements (WHILE, FOR, DO, GOTO, etc.) and processes them through the regular CFG pipeline
   - Adds non-control-flow statements directly to the current block
   - Prevents double-processing of statements

2. **Updated `processIfStatement()`**:
   - Calls `processNestedStatements()` for THEN branch
   - Calls `processNestedStatements()` for each ELSEIF clause
   - Calls `processNestedStatements()` for ELSE branch
   - Properly propagates line numbers to nested statements

3. **Header Declaration**: Added method signature to `fasterbasic_cfg.h`

### Why This Works

- Nested WHILE loops now get proper CFG blocks (loop header, body, exit)
- Back-edges are correctly created from WEND to loop header
- All control-flow structures (FOR, DO, REPEAT, etc.) inside IF blocks are now visible to the CFG builder
- Code generation produces correct control flow with proper loops

### Verification

After the fix:
- `tests/test_while_if_nested.bas` - ✅ Now works correctly (inner loop iterates multiple times)
- `tests/test_primes_sieve.bas` - ✅ Now works correctly (sieve algorithm completes)
- `tests/test_while_nested_simple.bas` - ✅ Still works (no regression)

CFG trace (`./qbe_basic -G`) now shows proper loop structure:
```
Block 6 (WHILE Loop Header) [LOOP HEADER]
  [16] WHILE (line 1120) - creates loop
  Successors: 7, 8

Block 7 (WHILE Loop Body)
  [17] PRINT (line 1120)
  [18] LET/ASSIGNMENT (line 1120)
  [19] LET/ASSIGNMENT (line 1120)
  [20] WEND (line 1120) - ends loop
  Successors: 6  ← Proper back-edge!
```

## Impact (Historical)

This bug **affected** (now fixed):
- Prime sieve algorithms
- Any nested loop inside conditional logic
- Complex control flow patterns

It did NOT affect:
- Simple programs without nested structures
- Programs using GOTO for control flow
- Single-line IF statements

## Testing

Test cases verified after fix:
- `tests/test_while_nested_simple.bas` - ✅ Works (no IF) - no regression
- `tests/test_while_if_nested.bas` - ✅ Now works correctly (WHILE inside IF)
- `tests/test_primes_sieve.bas` - ✅ Now works correctly (real-world example)

All tests produce correct output with proper loop iteration counts.

## CFG Trace Analysis

Using `./qbe_basic -G file.bas` to trace CFG construction reveals the issue:

### Working Case (Simple Nested WHILE):
```
Block 1 (WHILE Loop Header)
  [4] WHILE (outer)
  Successors: 2, 6

Block 2 (WHILE Loop Body)
  [5] PRINT
  [6] LET/ASSIGNMENT
  Successors: 3

Block 3 (WHILE Loop Header)  ← Inner loop visible!
  [7] WHILE (inner)
  Successors: 4, 5

Block 4 (WHILE Loop Body)
  [8] PRINT
  [9] LET/ASSIGNMENT
  [10] WEND (inner)
  Successors: 3
```

### Failing Case (WHILE Inside IF):
```
Block 4 (WHILE Loop Header)
  [10] WHILE (outer)
  Successors: 5, 6

Block 5 (WHILE Loop Body)
  [11] PRINT
  [12] IF - then:9 else:0  ← Inner WHILE trapped here!
  [13] LET/ASSIGNMENT
  [14] WEND (outer)
  Successors: 4

NO Block 6 (inner WHILE header) - it's missing!
```

The IF statement shows `then:9`, meaning it has 9 nested statements as children (including the inner WHILE and WEND). These statements are **not processed by the CFG builder** and don't get their own blocks.

## References

- Source: `fsh/FasterBASICT/src/fasterbasic_cfg.cpp`
- Function: `CFGBuilder::processIfStatement()` (line ~456)
- Related: `CFGBuilder::processStatement()` (line ~254)
- Related: `CFGBuilder::buildEdges()` (WHILE/WEND matching logic)
- Tracing: Use `./qbe_basic -G file.bas` to dump CFG structure