# Nested Control Flow Test Results - February 1, 2025

**Date:** February 1, 2025  
**Status:** ⚠️ **CRITICAL BUGS FOUND**  
**Test Suite:** Nested Control Flow Validation  
**Result:** 3 of 6 tests PASS, 3 of 6 tests FAIL (infinite loops)

---

## Executive Summary

Ran comprehensive nested control flow test suite and **discovered critical CFG builder bugs**:

- ✅ **WHILE in IF:** Works correctly
- ✅ **FOR in IF:** Works correctly
- ❌ **REPEAT in IF:** **INFINITE LOOP** - loops don't iterate correctly
- ❌ **DO LOOP in IF:** **INFINITE LOOP** - loops don't iterate correctly
- ❌ **Mixed nesting:** **INFINITE LOOP** - complex nesting fails

**Root Cause:** CFG builder does not properly handle REPEAT and DO LOOP statements inside IF branches. The previous fix for WHILE/FOR nesting (Jan 2025) did not extend to REPEAT/DO constructs.

---

## Test Results Summary

| Test File | Status | Issue |
|-----------|--------|-------|
| test_nested_while_if.bas | ✅ PASS | All 6 tests pass |
| test_nested_if_while.bas | ✅ PASS | All 6 tests pass |
| test_nested_for_if.bas | ✅ PASS | All 15 tests pass |
| test_nested_repeat_if.bas | ❌ FAIL | Infinite loop in Test 2 |
| test_nested_do_if.bas | ❌ FAIL | Infinite loop in Test 2 |
| test_nested_mixed_controls.bas | ❌ FAIL | Infinite loop in early tests |

**Pass Rate:** 50% (3 of 6)  
**Critical Issues:** 3 infinite loop bugs

---

## Detailed Test Results

### ✅ PASS: test_nested_while_if.bas

**Status:** All 6 tests pass  
**Execution Time:** < 1 second  
**Output:** Clean, all expected iterations complete

**Sample Output:**
```
=== Test 6: Multiple WHILEs in same IF ===
First WHILE:
  A: 1
  A: 2
  A: 3
Second WHILE:
  B: 10
  B: 11
  B: 12
Test 6 complete

=== All nested WHILE-IF tests passed ===
```

**Verdict:** WHILE loops inside IF statements work correctly after Jan 2025 fix.

---

### ✅ PASS: test_nested_if_while.bas

**Status:** All 6 tests pass  
**Execution Time:** < 1 second  
**Output:** Clean, all conditions evaluate correctly

**Sample Output:**
```
=== Test 6: IF affecting WHILE control ===
Counting: 1
Counting: 2
Counting: 3
Counting: 4
Counting: 5
Breaking at i=6
Test 6 complete - counted to 5

=== All nested IF-WHILE tests passed ===
```

**Verdict:** IF statements inside WHILE loops work correctly.

---

### ✅ PASS: test_nested_for_if.bas

**Status:** All 15 tests pass  
**Execution Time:** < 1 second  
**Output:** Clean, all FOR loops iterate correctly with all STEP variants

**Sample Output:**
```
=== Test 15: Triple nesting - FOR in IF in FOR in IF ===
Triple nested: o=1 m=1 i=1
Triple nested: o=1 m=1 i=2
Triple nested: o=1 m=1 i=3
Test 15 complete

=== All nested FOR-IF tests passed ===
```

**Verdict:** FOR loops inside IF statements work correctly, including negative STEP and EXIT FOR.

---

### ❌ FAIL: test_nested_repeat_if.bas

**Status:** INFINITE LOOP in Test 2  
**Symptom:** Loop variable doesn't increment, stuck printing same value  
**Location:** REPEAT loop inside IF ELSE branch

**Failing Code:**
```basic
PRINT "=== Test 2: REPEAT in IF ELSE ==="
outer% = 1
REPEAT
    IF outer% > 5 THEN
        PRINT "  Should not see this"
    ELSE
        inner% = 10
        REPEAT
            PRINT "  Else branch: "; inner%
            inner% = inner% + 1        ' ← Never executes!
        UNTIL inner% > 12
    END IF
    outer% = outer% + 1
UNTIL outer% > 2
```

**Observed Output (repeats forever):**
```
=== Test 2: REPEAT in IF ELSE ===
Outer: 1
  Else branch: 10
  Else branch: 10
  Else branch: 10
  Else branch: 10
  Else branch: 10
  ... (infinite)
```

**Expected Output:**
```
=== Test 2: REPEAT in IF ELSE ===
Outer: 1
  Else branch: 10
  Else branch: 11
  Else branch: 12
Outer: 2
  Else branch: 10
  Else branch: 11
  Else branch: 12
Test 2 complete
```

**Analysis:**
- ✅ Test 1 (REPEAT in IF THEN) passes
- ❌ Test 2 (REPEAT in IF ELSE) fails - infinite loop
- The ELSE branch is reached (we see output)
- The `inner% = inner% + 1` line never executes
- Loop variable stuck at initial value (10)

**Root Cause:** CFG builder doesn't properly process REPEAT loop body inside IF ELSE branch.

---

### ❌ FAIL: test_nested_do_if.bas

**Status:** INFINITE LOOP in Test 2  
**Symptom:** Identical to REPEAT issue - loop variable doesn't increment  
**Location:** DO UNTIL loop inside IF ELSE branch

**Failing Code:**
```basic
PRINT "=== Test 2: DO UNTIL in IF ELSE ==="
outer% = 1
DO UNTIL outer% > 3
    IF outer% > 5 THEN
        PRINT "  Should not see this"
    ELSE
        inner% = 10
        DO UNTIL inner% > 12
            PRINT "  Else branch: "; inner%
            inner% = inner% + 1        ' ← Never executes!
        LOOP
    END IF
    outer% = outer% + 1
LOOP
```

**Observed Output (repeats forever):**
```
=== Test 2: DO UNTIL in IF ELSE ===
  Else branch: 10
  Else branch: 10
  Else branch: 10
  ... (infinite)
```

**Expected Output:**
```
=== Test 2: DO UNTIL in IF ELSE ===
  Else branch: 10
  Else branch: 11
  Else branch: 12
  Else branch: 10
  Else branch: 11
  Else branch: 12
Test 2 complete
```

**Root Cause:** Same as REPEAT - CFG builder doesn't process DO LOOP body inside IF ELSE branch.

---

### ❌ FAIL: test_nested_mixed_controls.bas

**Status:** INFINITE LOOP in early tests  
**Symptom:** Test doesn't complete, hangs early  
**Location:** Mixed DO/REPEAT with IF structures

**Observed:** Timeout after 5 seconds, incomplete output

**Root Cause:** Combination of DO and REPEAT bugs causes failure in mixed scenarios.

---

## Bug Pattern Analysis

### Common Characteristics

All failing tests share these patterns:

1. **Loop Type:** REPEAT or DO (not WHILE or FOR)
2. **Location:** Inside IF **ELSE** branch (not THEN branch)
3. **Symptom:** Loop variable doesn't update
4. **Behavior:** Infinite loop printing same value

### What Works

- ✅ WHILE in IF THEN
- ✅ WHILE in IF ELSE
- ✅ FOR in IF THEN
- ✅ FOR in IF ELSE
- ✅ IF inside any loop type

### What Fails

- ❌ REPEAT in IF ELSE
- ❌ DO UNTIL in IF ELSE
- ❌ DO WHILE in IF ELSE (likely)
- ❌ DO...LOOP variants in IF ELSE (likely)

### Hypothesis

The January 2025 fix (`processNestedStatements()`) correctly handles WHILE and FOR loops but does **not** handle REPEAT and DO loop constructs. These may use different AST node types or statement processing paths.

**Evidence:**
```cpp
// In processNestedStatements() - checks for control flow types
bool isControlFlow = (
    nodeType == "WhileStatement" ||    // ✅ Works
    nodeType == "ForStatement" ||      // ✅ Works
    nodeType == "DoStatement" ||       // ❌ Might not match
    nodeType == "RepeatStatement" ||   // ❌ Might not match
    ...
);
```

Likely issues:
1. REPEAT/DO statement types not in the `isControlFlow` check
2. REPEAT/DO processing functions not called recursively
3. Different CFG building path for REPEAT/DO vs WHILE/FOR

---

## CFG Structure Analysis

### Needed: CFG Trace Inspection

To diagnose further, we need to inspect CFG traces:

```bash
./qbe_basic -G tests/loops/test_nested_repeat_if.bas > repeat_cfg.txt
./qbe_basic -G tests/loops/test_nested_do_if.bas > do_cfg.txt
```

**Expected in CFG (correct):**
```
Block X (IF ELSE Branch)
  [N] IF - else branch
  Successors: Y

Block Y (REPEAT Loop Header) [LOOP HEADER]
  [N+1] REPEAT - creates loop
  Successors: Z, ...

Block Z (REPEAT Loop Body)
  [N+2] PRINT
  [N+3] LET/ASSIGNMENT    ← inner% = inner% + 1
  [N+4] UNTIL
  Successors: Y  ← Back-edge
```

**Suspected in CFG (bug):**
```
Block X (IF ELSE Branch)
  [N] IF - else branch
  [N+1] REPEAT  ← No header block!
  [N+2] PRINT
  [N+3] UNTIL   ← Missing LET statement!
  Successors: ...  ← No back-edge!
```

The LET statement (`inner% = inner% + 1`) is likely not being added to the loop body block.

---

## Recommended Fix

### Step 1: Identify Statement Types

Check the actual AST node type names for REPEAT and DO:

```cpp
// In fasterbasic_ast.h or similar
struct RepeatStatement : Statement { ... };
struct DoStatement : Statement { ... };
```

### Step 2: Update processNestedStatements()

Ensure REPEAT and DO are in the control flow check:

```cpp
bool isControlFlow = (
    stmt.type == StatementType::While ||
    stmt.type == StatementType::For ||
    stmt.type == StatementType::Do ||        // Add this
    stmt.type == StatementType::Repeat ||    // Add this
    stmt.type == StatementType::If ||
    // ... other types
);
```

### Step 3: Verify Processing Functions Exist

Ensure these functions exist and are called:

```cpp
void CFGBuilder::processRepeatStatement(const RepeatStatement& stmt, ...);
void CFGBuilder::processDoStatement(const DoStatement& stmt, ...);
```

### Step 4: Test Fix

After fix, all 6 test files should pass:

```bash
./scripts/test_nested_control_flow.sh
# Expected: 6 passed, 0 failed
```

---

## Verification Steps

### Before Fix

```bash
# These should FAIL (infinite loop, timeout after 30s)
./qbe_basic -o /tmp/test tests/loops/test_nested_repeat_if.bas
timeout 5 /tmp/test  # Times out

./qbe_basic -o /tmp/test tests/loops/test_nested_do_if.bas
timeout 5 /tmp/test  # Times out
```

### After Fix

```bash
# These should PASS (complete < 1 second)
./qbe_basic -o /tmp/test tests/loops/test_nested_repeat_if.bas
/tmp/test  # Completes with "All ... tests passed"

./qbe_basic -o /tmp/test tests/loops/test_nested_do_if.bas
/tmp/test  # Completes with "All ... tests passed"
```

---

## Impact Assessment

### Severity: **CRITICAL**

**Why Critical:**
1. Infinite loops are a severe bug (hangs programs)
2. Affects common BASIC constructs (REPEAT, DO LOOP)
3. Silent failure (no error, just hangs)
4. Data corruption risk (variables not updating)

### User Impact

Any FasterBASIC program with:
```basic
IF condition THEN
    ' ...
ELSE
    REPEAT
        ' This code won't execute properly!
    UNTIL done
END IF
```

Will hang indefinitely.

### Workaround

Until fixed, users can avoid:
- REPEAT loops inside IF ELSE branches
- DO LOOP variants inside IF ELSE branches

Use WHILE loops instead:
```basic
IF condition THEN
    ' ...
ELSE
    WHILE NOT done
        ' This works!
    WEND
END IF
```

---

## Files to Modify

Based on January 2025 fix patterns:

1. **fsh/FasterBASICT/src/fasterbasic_cfg.cpp**
   - Update `processNestedStatements()` to handle REPEAT/DO
   - Ensure recursive processing for all loop types
   - Add REPEAT/DO to control flow checks

2. **fsh/FasterBASICT/src/fasterbasic_cfg.h**
   - Verify declarations for all loop processing functions

3. **Test with:**
   - `tests/loops/test_nested_repeat_if.bas`
   - `tests/loops/test_nested_do_if.bas`
   - `tests/loops/test_nested_mixed_controls.bas`

---

## Related Issues

### Previously Fixed (Jan 2025)

- ✅ WHILE in IF executing only once
- ✅ FOR in IF not iterating properly

**Fix:** Implemented `processNestedStatements()` for recursive CFG building

**Files Changed:** `fasterbasic_cfg.{h,cpp}`

**Documentation:** `docs/session_notes/CFG_FIX_SESSION_COMPLETE.md`

### Currently Broken (Feb 2025)

- ❌ REPEAT in IF ELSE - infinite loop
- ❌ DO LOOP in IF ELSE - infinite loop
- ❌ Mixed REPEAT/DO nesting - infinite loop

**Root Cause:** Same as previous issue, but fix didn't cover REPEAT/DO

**Priority:** HIGH - Critical bug causing hangs

---

## Testing Checklist

After implementing fix:

- [ ] test_nested_while_if.bas - should still pass
- [ ] test_nested_if_while.bas - should still pass
- [ ] test_nested_for_if.bas - should still pass
- [ ] test_nested_repeat_if.bas - should now pass
- [ ] test_nested_do_if.bas - should now pass
- [ ] test_nested_mixed_controls.bas - should now pass
- [ ] Run full test suite - no regressions
- [ ] Inspect CFG traces - proper structure
- [ ] Test real-world programs with REPEAT/DO nesting

---

## Next Steps

1. **Immediate:** Generate CFG traces for failing tests
   ```bash
   ./qbe_basic -G tests/loops/test_nested_repeat_if.bas > repeat_cfg.txt
   ./qbe_basic -G tests/loops/test_nested_do_if.bas > do_cfg.txt
   ```

2. **Investigate:** Identify how REPEAT/DO are processed differently from WHILE/FOR

3. **Fix:** Update CFG builder to handle REPEAT/DO recursively

4. **Test:** Verify all 6 nested tests pass

5. **Document:** Update CFG_FIX_SESSION_COMPLETE.md with REPEAT/DO fix details

6. **Deploy:** Merge fix and update release notes

---

## Conclusion

The nested control flow test suite **successfully identified critical CFG builder bugs**:

✅ **Mission Accomplished:** Tests found the fragile areas  
⚠️ **Bugs Found:** REPEAT and DO loops in IF ELSE branches cause infinite loops  
📊 **Success Rate:** 50% pass rate before fix  
🎯 **Target:** 100% pass rate after fix

These tests have proven their value by immediately exposing serious CFG builder fragility issues that would have caused production bugs and user frustration.

---

**Test Run Date:** February 1, 2025  
**Engineer:** AI Assistant  
**Test Suite Version:** 1.0  
**Status:** BUGS FOUND - FIX REQUIRED  
**Priority:** HIGH - Critical infinite loop bugs

---

## Appendix: Full Test Output

### test_nested_repeat_if.bas (truncated - infinite loop)
```
=== Test 1: REPEAT in IF THEN ===
  Inner: 1
  Inner: 2
  Inner: 3
  Inner: 4
  Inner loop ran 4 times
Outer: 1
Outer: 2
Outer: 3
Test 1 complete

=== Test 2: REPEAT in IF ELSE ===
Outer: 1
  Else branch: 10
  Else branch: 10
  Else branch: 10
  ... (infinite, killed by timeout)
```

### test_nested_do_if.bas (truncated - infinite loop)
```
=== Test 1: DO WHILE in IF THEN ===
  Inner: 1
  Inner: 2
  Inner: 3
  Inner: 4
  Inner loop ran 4 times
Outer: 1
Outer: 2
Outer: 3
Test 1 complete

=== Test 2: DO UNTIL in IF ELSE ===
  Else branch: 10
  Else branch: 10
  Else branch: 10
  ... (infinite, killed by timeout)
```

Both show same pattern: Test 1 (THEN branch) passes, Test 2 (ELSE branch) hangs.