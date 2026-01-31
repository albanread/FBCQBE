# FOR Loop Improvements - Phase 4

## Date
January 26, 2025

## Summary
Implemented proper FOR loop handling with internal loop counters separate from user variables.

## Changes Made

### 1. AST Changes (`fasterbasic_ast.h`)

Added fields to `ForStatement` class:
- `internalCounter` - Internal counter variable name (e.g., "for_i")
- `useWord` - true = use 32-bit int (w), false = use 64-bit long (l)
- `limitIsConstant` - true if end expression is a constant
- `constantLimit` - Value of end if it's a constant

### 2. Semantic Analyzer Changes (`fasterbasic_semantic.cpp`)

Modified `validateForStatement()`:
- Analyzes the loop limit expression to determine if it's constant
- If constant and fits in 32-bit range (-2147483648 to 2147483647), uses 32-bit int counter
- Otherwise uses 64-bit long counter
- Generates internal counter name: `for_` + variable name
- Does NOT register the user's loop variable in symbol table (loop counter is internal)

### 3. CFG Changes (`fasterbasic_cfg.h`, `fasterbasic_cfg.cpp`)

Updated `ForLoopBlocks` structure:
- Added `internalCounter` field
- Added `useWord` field
- Updated `processForStatement()` to copy these fields from AST to CFG structure

### 4. Code Generator Changes (`qbe_codegen_statements.cpp`, `qbe_codegen_main.cpp`)

#### `emitFor()` - FOR initialization
- Uses `%for_` prefix for internal counters (e.g., `%for_i`, `%for_j`)
- Determines counter type: `w` (32-bit) or `l` (64-bit) based on `stmt->useWord`
- Converts start/end/step expressions to match counter type
- All loop variables (counter, step, end) use the same type

#### FOR Loop Check Block
- Uses internal counter for comparisons
- Uses appropriate comparison instruction:
  - `cslew` for 32-bit signed comparison
  - `cslel` for 64-bit signed comparison

#### `emitNext()` - NEXT statement
- Finds matching FOR loop structure from CFG
- Uses internal counter and type from ForLoopBlocks
- Increments counter using appropriate type

## Results

### Test Case
```basic
FOR i = 1 TO 10
    total = total + i
NEXT i
```

### Generated IL (Key Parts)
```qbe
# FOR i = start TO end [counter type: w]
%for_i =w copy %t3          # Internal counter is 32-bit int
%step_for_i =w copy %t4     # Step is 32-bit int
%end_for_i =w copy %t6      # End is 32-bit int

# Loop check
%t7 =w cslew %for_i, %end_for_i    # 32-bit comparison
jnz %t7, @block_3, @block_4

# Loop increment
%t9 =w add %for_i, %step_for_i     # 32-bit arithmetic
%for_i =w copy %t9
```

## Benefits Achieved

✅ **Simple counter names**: `%for_i` instead of `%var_i_BYTE`
✅ **Type optimization**: Uses 32-bit int for small ranges (more efficient)
✅ **Consistent types**: Counter, step, and end all use same type
✅ **Proper comparisons**: Uses appropriate comparison instruction for type
✅ **No type suffix pollution**: Loop counters don't carry user's type suffixes

## Remaining Issues

### ⚠️ Issue #1: User Variable References Inside Loop

**Problem**: When user references the loop variable inside the loop body, it refers to `%var_i` (which may be a different type) instead of the internal counter `%for_i`.

**Example**:
```basic
DIM i@ AS BYTE
FOR i@ = 1 TO 10
    PRINT i@    ' This prints %var_i_BYTE, not %for_i!
NEXT i@
```

**Current Behavior**:
- `%for_i` is incremented correctly (as 32-bit int)
- `%var_i_BYTE` is never updated
- User sees wrong value or uninitialized variable

**Solution Needed**:
Option A: Make loop variable references inside FOR loops map to the internal counter
- Requires scope tracking in expression evaluation
- Variable reference `i` inside loop → `%for_i`
- Variable reference `i` outside loop → `%var_i_BYTE`

Option B: Copy internal counter to user variable at each iteration
- Add assignment in loop body: `%var_i_BYTE =l copy %for_i`
- Less efficient but simpler to implement
- Preserves user's variable type semantics

Option C: Treat FOR loop variable as special - don't allow it to be declared
- FOR creates its own temporary integer variable
- User cannot DIM the loop variable
- Most restrictive but clearest semantics

### ⚠️ Issue #2: Large Constant Limits

**Problem**: Limits > 32-bit switch to 64-bit long, but this needs testing.

**Test Needed**:
```basic
FOR i = 1 TO 5000000000
    ' Should use 64-bit counter
NEXT i
```

### ⚠️ Issue #3: Non-Constant Limits

**Problem**: Currently defaults to 64-bit for all non-constant limits.

**Potential Optimization**:
- Could analyze expression type
- If both start and end are known to be small integers, use 32-bit
- Requires type range analysis

### ⚠️ Issue #4: Step Direction

**Current**: Always uses `cslel/cslew` (signed less-than-or-equal)

**Problem**: Doesn't handle negative STEP correctly.
- For `FOR i = 10 TO 1 STEP -1`, should use >= comparison
- Current code will never enter loop

**Solution**: 
- Analyze STEP sign at compile time if constant
- Use `csgel/csgew` for negative STEP
- For variable STEP, need runtime decision or different loop structure

## Recommendations

### Immediate (Required for Correctness)

1. **Fix Issue #1** - Choose and implement solution for user variable references
   - Recommend Option A (scope-based mapping) for best performance
   - Or Option B (copy per iteration) for simpler implementation

2. **Fix Issue #4** - Handle negative STEP
   - Check STEP sign during semantic analysis
   - Store in ForStatement and use appropriate comparison

### Short Term (Optimization)

3. **Test Issue #2** - Verify 64-bit counter generation works
4. **Optimize Issue #3** - Better type inference for non-constant limits

### Long Term (Enhancement)

5. Add overflow detection for loop counters
6. Support floating-point loop variables (some BASICs allow this)
7. Optimize away dead loop variables (when not referenced in body)

## Testing Recommendations

Create test cases for:
- [x] Simple loop with 32-bit limit
- [x] Loop with 32-bit arithmetic
- [ ] Loop with user variable reference in body
- [ ] Loop with 64-bit limit (> 2^31)
- [ ] Loop with negative STEP
- [ ] Nested loops
- [ ] Loop with user-typed variable (BYTE/SHORT)
- [ ] Loop exit and continue statements

## Conclusion

Significant progress made on FOR loop implementation:
- Internal counters now use simple names
- Type selection based on range is working
- All loop components use consistent types

Critical bug remains: User variable references don't map to internal counter.
This must be fixed before Phase 4 can be considered complete.