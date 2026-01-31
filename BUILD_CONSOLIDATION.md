# Build Consolidation and IF Statement Fix

**Date**: January 31, 2026  
**Issue**: Multiline IF/THEN/ELSE/END IF statements not working correctly  
**Root Cause**: Multiple build locations with different code versions

## Problem Summary

The Levenshtein distance algorithm and other nested IF statement tests were failing, returning 0 instead of correct results. After investigation, we discovered this was NOT a logic bug in the code, but rather a **build configuration issue**.

### What Was Wrong

The project had **two different `qbe_basic` executables** in different locations:

1. **`./qbe_basic`** (project root)
   - Built: January 30, 2026 at 20:32
   - Status: OLD code with buggy IF logic
   - Result: All IF statement tests failed

2. **`./qbe_basic_integrated/qbe_basic`** (subdirectory)
   - Built: January 31, 2026 at 08:19
   - Status: NEW code with fixed IF logic
   - Result: All tests pass correctly

Different developers/sessions were accidentally using different executables, leading to inconsistent behavior.

## The Actual Code Bug (Already Fixed)

The bug existed in `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp` in the `emitIf()` function.

### Old Buggy Code

```cpp
bool isInlineIF = !stmt->isMultiLine && !stmt->thenStatements.empty();
bool isBlockLevelIF = stmt->isMultiLine || (m_currentBlock && m_currentBlock->successors.size() == 2);

if (isInlineIF) {
    // emit inline IF
} else if (isBlockLevelIF) {
    m_lastCondition = boolTemp;
    emitComment("Condition stored for CFG-driven branch to successors");
} else {
    // Fallback
    m_lastCondition = boolTemp;
}
```

**The Problem**: The `isBlockLevelIF` check tried to examine `m_currentBlock->successors.size()`, but this information isn't reliable during code generation because successors are populated by the CFG builder, not the code generator. This caused the wrong code path to be taken.

### Fixed Code (Uncommitted)

```cpp
bool isInlineIF = !stmt->isMultiLine && !stmt->thenStatements.empty();

if (isInlineIF) {
    // emit inline IF
} else {
    // Multi-line IF - store condition for CFG-driven branching
    m_lastCondition = boolTemp;
    emitComment("Multi-line IF: condition stored for CFG branching");
}
```

**The Fix**: Simplified the logic to just check `!isInlineIF`. If it's not an inline IF, it must be a multiline/CFG-driven IF, so store the condition. This removed the buggy `isBlockLevelIF` check entirely.

## How Multiline IF Statements Work

### Parser Phase
1. Parser detects `IF x THEN` followed by newline
2. Sets `stmt->isMultiLine = true`
3. Recursively parses nested statements into `stmt->thenStatements` and `stmt->elseStatements`

### CFG Builder Phase
1. `processIfStatement()` detects `stmt->isMultiLine == true`
2. Creates separate basic blocks:
   - Block for IF condition evaluation
   - Block for THEN branch
   - Block for ELSE branch  
   - Block for "After IF" (convergence point)
3. Recursively processes nested statements via `processNestedStatements()`
4. Connects blocks with conditional edges

### Code Generator Phase
1. `emitIf()` evaluates the condition
2. For multiline IF, stores condition in `m_lastCondition`
3. `emitBlock()` later uses `m_lastCondition` to emit conditional branch
4. Visits all blocks in order, emitting code for each

## Solution: Build Consolidation

### Changes Made

1. **Deleted old root-level `qbe_basic` executable**
   - This was built from old code

2. **Created symlink at project root**
   ```bash
   ln -s qbe_basic_integrated/qbe_basic qbe_basic
   ```
   - Now `./qbe_basic` always points to the latest build

3. **Removed duplicate build scripts**
   - Deleted `fsh/build_fbc_qbe.sh` (unused/old)
   - Only `qbe_basic_integrated/build_qbe_basic.sh` remains

4. **Created BUILD.md**
   - Clear instructions on how to build
   - Warns about using only the single build location

5. **Updated START_HERE.md and README.md**
   - Added prominent warnings about build location
   - Updated all build commands to use correct path

### Verification

After consolidation, all tests pass:

```bash
# Simple IF test
./qbe_basic test_if_minimal.bas
# Result = 1 (expected 1) ✓

# Nested IF test  
./qbe_basic test_nested_if_simple.bas
# All 3 test cases pass ✓

# Levenshtein distance (complex nested IFs in loops)
./qbe_basic tests/rosetta/levenshtein_distance.bas
# All 6 test cases pass ✓
```

## Lessons Learned

1. **Single Build Location**: Projects should have ONE canonical build location to avoid version confusion

2. **Symlinks for Convenience**: If developers need to access the binary from multiple locations, use symlinks rather than copying

3. **Build Verification**: After any code change, verify which executable is being used:
   ```bash
   ls -l ./qbe_basic  # Check if it's a symlink
   which qbe_basic    # Check what PATH resolves to
   ```

4. **Test Both Passes and Failures**: When debugging, test with both old and new builds to confirm the fix

5. **Uncommitted Fixes Can Hide**: The fix was already in the working directory but uncommitted, so old builds didn't have it

## Status

- ✅ Build consolidation complete
- ✅ All IF statement tests passing
- ✅ Documentation updated
- ⚠️ Code fix needs to be committed (in `qbe_codegen_statements.cpp`)

## Next Steps

1. Commit the `emitIf()` fix in `qbe_codegen_statements.cpp`
2. Add a test case for multiline IF statements to the test suite
3. Consider adding a pre-commit hook to verify only one qbe_basic exists