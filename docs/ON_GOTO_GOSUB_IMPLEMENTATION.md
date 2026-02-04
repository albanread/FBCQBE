# ON GOTO / ON GOSUB Implementation Summary

**Date:** February 4, 2025  
**Status:** ✅ IMPLEMENTED AND TESTED

## Overview

This document summarizes the implementation of BASIC `ON GOTO` and `ON GOSUB` computed jump statements in the FasterBASIC → QBE compiler backend.

## Problem Statement

The FasterBASIC compiler needed to support computed jump statements:
- `ON x GOTO line1, line2, line3` - Jump to one of several labels/line numbers based on selector value
- `ON x GOSUB line1, line2, line3` - Call one of several subroutines based on selector value

The AST already had `OnGotoStatement` and `OnGosubStatement` nodes, but the CFG builder and code generator did not handle them.

## Implementation Details

### 1. CFG Builder Dispatch Fix

**Issue:** The CFG builder's statement dispatch loops in `buildFromProgram()` and `buildProgramCFG()` did not include cases for `OnGotoStatement` and `OnGosubStatement`, so the specialized handlers were never invoked.

**Fix:** Added dispatch handlers in two files:

- `fsh/FasterBASICT/src/cfg/cfg_builder_core.cpp` (lines 219-229)
- `fsh/FasterBASICT/src/cfg/cfg_builder_functions.cpp` (lines 572-582)

```cpp
if (auto* onGotoStmt = dynamic_cast<const OnGotoStatement*>(stmt.get())) {
    currentBlock = handleOnGoto(*onGotoStmt, currentBlock);
    continue;
}

if (auto* onGosubStmt = dynamic_cast<const OnGosubStatement*>(stmt.get())) {
    currentBlock = handleOnGosub(*onGosubStmt, currentBlock, nullptr, nullptr, nullptr, nullptr);
    continue;
}
```

### 2. QBE Switch Instruction Fix

**Issue:** The original `QBEBuilder::emitSwitch()` method used bracket syntax `jmp @default [ %selector @case0 @case1 ]`, which does not exist in QBE. QBE only supports:
- `jmp @label` (unconditional)
- `jnz %val, @true, @false` (conditional)
- `ret [value]` (return)
- `hlt` (halt)

**Fix:** Rewrote `emitSwitch()` in `fsh/FasterBASICT/src/codegen_v2/qbe_builder.cpp` to generate a chain of conditional jumps:

```qbe
# For ON x GOTO L1, L2, L3 with selector already 0-indexed:
%t.4 =w ceqw %selector, 0
jnz %t.4, @L1, @switch_next_0
@switch_next_0
%t.5 =w ceqw %selector, 1
jnz %t.5, @L2, @switch_next_1
@switch_next_1
%t.6 =w ceqw %selector, 2
jnz %t.6, @L3, @default
```

This generates efficient code that QBE can optimize further.

### 3. Jump Target Block Creation Fix

**Issue:** When a numbered line (e.g., `200 PRINT "Hello"`) appeared after a terminator (like `END`), the CFG builder would create an "unreachable" block for the PRINT statement, then create a separate labeled block (`Line_200`) that was empty. ON GOTO would jump to the empty `Line_200` block, missing the actual code.

**Fix:** Updated the condition for creating jump target blocks in both `cfg_builder_core.cpp` and `cfg_builder_functions.cpp`:

```cpp
// OLD: Only create new block if current has statements or is entry
if (!currentBlock->statements.empty() || currentBlock == m_entryBlock) {

// NEW: Also create if current block is terminated
if (!currentBlock->statements.empty() || currentBlock == m_entryBlock || isTerminated(currentBlock)) {
```

This ensures that when we encounter a jump target line, we always create a proper labeled block for it, even if the previous block was terminated.

### 4. Deferred Edge Resolution Enhancement

**Issue:** The original `resolveDeferredEdges()` only handled line-number-based forward references. ON statements can also use label-based targets (though the current parser primarily uses line numbers).

**Fix:** Updated `cfg_builder_jumptargets.cpp` to check both `targetLabel` and `targetLineNumber`:

```cpp
if (!deferred.targetLabel.empty()) {
    targetBlock = resolveLabelToBlock(deferred.targetLabel);
    // ... add edge if found
} else {
    targetBlock = resolveLineNumberToBlock(deferred.targetLineNumber);
    // ... add edge if found
}
```

## Code Generation Strategy

### ON GOTO
1. Evaluate selector expression to machine word
2. Subtract 1 (convert BASIC 1-based to 0-based)
3. Generate comparison chain for each target
4. If no match, fall through to next statement

### ON GOSUB
1. Evaluate selector expression to machine word
2. Subtract 1 (convert BASIC 1-based to 0-based)
3. Generate comparison chain targeting trampolines
4. Each trampoline:
   - Pushes return block ID onto gosub stack
   - Jumps to the actual subroutine target
5. RETURN pops the stack and jumps back

## Test Results

### ✅ test_on_goto.bas (comprehensive)
All 10 tests passed:
- Basic indexing (1, 2, 3)
- Computed expressions (5-3=2)
- Edge cases (index 0, negative, out-of-range)
- Single target
- Many targets (6 options)
- Complex expressions (MOD operator)

### ✅ test_on_gosub.bas (comprehensive)
All 11 tests passed:
- Basic indexing (1, 2, 3)
- Computed expressions (7-5=2)
- Edge cases (index 0, negative, out-of-range)
- Single target
- Many targets (6 options)
- **Nested GOSUB calls** - multiple levels work correctly
- Execution continues after RETURN

### Sample Programs

**Simple ON GOTO:**
```basic
LET x = 2
ON x GOTO 100, 200, 300
100 PRINT "First": END
200 PRINT "Second": END  ' This executes
300 PRINT "Third": END
```

**Simple ON GOSUB:**
```basic
LET x = 2
ON x GOSUB 100, 200, 300
PRINT "Back in main"
END
100 PRINT "Sub 1": RETURN
200 PRINT "Sub 2": RETURN  ' This executes, then returns
300 PRINT "Sub 3": RETURN
```

## Performance Characteristics

- **Small switch (< 5 cases):** Linear comparison chain is efficient
- **Large switch (10+ cases):** Still competitive; QBE may optimize to jump table at backend
- **Selector evaluation:** Done once, cached in temporary
- **GOSUB overhead:** One stack push per call (same as direct GOSUB)

## Known Limitations

1. **Line number format:** BASIC programs must have statements on the same line as their line number:
   ```basic
   200 PRINT "Hello": END    ' GOOD
   ```
   Not on separate lines:
   ```basic
   200
   PRINT "Hello"             ' BAD - parser issue (unrelated to ON implementation)
   ```

2. **Maximum targets:** Limited by available memory for CFG edges (practically unlimited)

3. **Selector type:** Automatically converts FLOAT/DOUBLE to INTEGER via truncation

## Files Modified

1. **CFG Builder:**
   - `fsh/FasterBASICT/src/cfg/cfg_builder_core.cpp` - Added ON dispatch
   - `fsh/FasterBASICT/src/cfg/cfg_builder_functions.cpp` - Added ON dispatch
   - `fsh/FasterBASICT/src/cfg/cfg_builder_jumptargets.cpp` - Enhanced edge resolution
   - `fsh/FasterBASICT/src/cfg/cfg_builder_jumps.cpp` - ON handlers (already existed, now invoked)

2. **Code Generator:**
   - `fsh/FasterBASICT/src/codegen_v2/qbe_builder.cpp` - Rewrote emitSwitch()
   - `fsh/FasterBASICT/src/codegen_v2/cfg_emitter.cpp` - ON terminators (already existed)

## Future Enhancements

1. **Optimization:** For very large switch statements (20+ targets), consider generating a computed-goto pattern with a jump table in the data segment

2. **Range checking:** Optional bounds check with runtime error instead of silent fallthrough

3. **Label-based targets:** Full support for `ON x GOTO label1, label2` in addition to line numbers (parser enhancement needed)

## References

- Original implementation plan: Thread "ON GOTO GOSUB Implementation Plan"
- Test suite: `tests/functions/test_on_goto.bas`, `tests/functions/test_on_gosub.bas`
- QBE IL specification: `qbe_basic_integrated/qbe_source/doc/il.txt`

---

**Implementation by:** FasterBASIC Development Team  
**Compiler Version:** v2 (CFG-aware code generation)  
**Last Updated:** February 4, 2025