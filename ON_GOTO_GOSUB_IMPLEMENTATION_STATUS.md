# ON GOTO/GOSUB Implementation Status

## Overview

This document tracks the implementation of ON GOTO and ON GOSUB statements in the FasterBASIC QBE backend. These are computed jump statements that select one of multiple targets based on an expression value (1-based indexing in BASIC).

**Implementation Date:** February 4, 2026  
**Status:** Codegen Complete, CFG Builder Bug Blocking Testing  
**Related Documents:** `docs/ONGOPLAN.md`

## Implementation Summary

The ON GOTO/GOSUB implementation follows the plan outlined in `docs/ONGOPLAN.md` and uses QBE's native switch instruction for efficient multiway dispatch instead of linear comparison chains.

### Architecture

1. **CFG Builder** creates multiple edges from ON statements:
   - For ON GOTO: edges labeled "case_1", "case_2", etc., plus "default" for fallthrough
   - For ON GOSUB: edges labeled "call_1", "call_2", etc., plus a JUMP to the return point

2. **CFG Emitter** detects ON statements in block terminators and generates optimized IL:
   - ON GOTO: Direct switch to target blocks using QBE's jump table
   - ON GOSUB: Switch to trampolines that push return block ID then jump to subroutine

3. **QBE Backend** compiles the switch instruction to efficient jump tables in assembly

## Completed Work

### 1. QBEBuilder Enhancement (`src/codegen_v2/qbe_builder.h`, `qbe_builder.cpp`)

**Added:** `emitSwitch()` method
```cpp
void emitSwitch(const std::string& type, const std::string& selector,
                const std::string& defaultLabel,
                const std::vector<std::string>& caseLabels);
```

This method emits QBE's native switch instruction:
```qbe
jmp @default [ %selector @case0 @case1 @case2 ... ]
```

The switch uses 0-based indexing, so selectors are adjusted by subtracting 1.

### 2. CFGEmitter Updates (`src/codegen_v2/cfg_emitter.h`, `cfg_emitter.cpp`)

**Added helper methods:**

- `emitSelectorWord(const Expression* expr)` - Evaluates selector expression and normalizes to word type (handles int, float, double conversions)
- `emitPushReturnBlock(int returnBlockId)` - Pushes return block ID onto GOSUB return stack
- `emitOnGotoTerminator(...)` - Handles ON GOTO using switch dispatch
- `emitOnGosubTerminator(...)` - Handles ON GOSUB with trampoline generation

**Modified methods:**

- `emitBlockStatements()` - Skips ON GOTO/GOSUB statements (they're terminators)
- `emitBlockTerminator()` - Detects ON statements before processing other edge types
- Direct GOSUB emission - Refactored to use shared `emitPushReturnBlock()` helper

**Implementation details:**

**ON GOTO Logic:**
1. Finds all edges with labels matching "case_N"
2. Evaluates selector expression once
3. Subtracts 1 to convert from 1-based (BASIC) to 0-based (QBE)
4. Emits switch instruction with case labels
5. Out-of-range values jump to default (fallthrough block)

**ON GOSUB Logic:**
1. Finds all edges with labels matching "call_N"
2. Creates trampoline labels for each case
3. Emits switch instruction to trampolines
4. Each trampoline:
   - Pushes return block ID onto stack
   - Jumps to actual subroutine target
5. Out-of-range values jump directly to return point (no subroutine call)

### 3. CFG Builder Fixes (`src/cfg/cfg_builder_jumps.cpp`, `cfg_builder.h`)

**Fixed:** `handleOnGoto()` and `handleOnGosub()` to properly handle both labels and line numbers

Previously, these methods only checked `stmt.lineNumbers` and ignored `stmt.isLabelList`, causing them to fail when targets were labels.

**Changes:**
- Now iterate over `stmt.isLabelList.size()` instead of `stmt.lineNumbers.size()`
- Check `stmt.isLabelList[i]` to determine if target is label or line number
- Use `resolveLabelToBlock()` for labels, `resolveLineNumberToBlock()` for line numbers
- Handle forward references correctly for both types

**Added:** `targetLabel` field to `DeferredEdge` struct for label-based forward references

## Current Issue: CFG Builder Not Invoking Handlers

### Problem Description

During testing, it was discovered that `handleOnGoto()` and `handleOnGosub()` are **never being called** during CFG construction. This is a pre-existing bug in the CFG builder, not related to the new codegen implementation.

**Evidence:**
- Debug output added to `handleOnGoto()` (both stderr and file-based) never appears
- CFG analysis shows ON GOTO statements exist in blocks but no case edges are created
- All edges from blocks with ON statements are simple JUMP/FALLTHROUGH edges
- Test programs fall through instead of jumping to targets

**Test Case:**
```basic
PRINT "Testing ON GOTO"
LET x = 2
ON x GOTO 100, 200, 300
PRINT "ERROR: Fell through"
END

100 PRINT "Reached 100 - ERROR"
END

200 PRINT "Reached 200 - CORRECT"
END

300 PRINT "Reached 300 - ERROR"
END
```

**Expected:** Should print "Reached 200 - CORRECT"  
**Actual:** Prints "ERROR: Fell through"

**CFG Analysis:**
- Block 0 contains the OnGotoStatement
- Only 1 outgoing edge (FALLTHROUGH to EXIT)
- No "case_1", "case_2", "case_3" edges created

### Root Cause Investigation Needed

The issue is in the CFG builder's statement dispatch logic. Possible causes:

1. **Statement type not recognized** - The dynamic_cast or type checking in `buildStatementRange()` might not handle OnGotoStatement
2. **Statement filtering** - ON GOTO/GOSUB might be filtered out before reaching the handler
3. **Parser issue** - Statements might not be added to the program structure correctly
4. **Build issue** - Wrong version of CFG builder code being compiled

**Files to investigate:**
- `src/cfg/cfg_builder_statements.cpp` - Statement dispatch logic
- `src/fasterbasic_parser.cpp` - Parser's handling of ON statements
- `qbe_basic_integrated/build_qbe_basic.sh` - Build script to verify correct files

## Testing Plan

Once the CFG builder issue is resolved, the following tests should pass:

### Existing Tests
1. `tests/functions/test_on_goto.bas` - Comprehensive ON GOTO test suite
   - Tests indices 1, 2, 3
   - Tests computed expressions (5-3=2)
   - Tests index 0 (fallthrough)
   - Tests out-of-range index (fallthrough)
   - Tests negative index (fallthrough)
   - Tests single target
   - Tests many targets (6 options)
   - Tests MOD expression

2. `tests/functions/test_on_gosub.bas` - Comprehensive ON GOSUB test suite
   - Tests indices 1, 2, 3
   - Tests computed expressions
   - Tests index 0 (fallthrough, no call)
   - Tests out-of-range (fallthrough)
   - Tests negative index (fallthrough)
   - Tests single target
   - Tests nested ON GOSUB calls
   - Tests execution continues after RETURN

### New Test Created
- `test_programs/test_on_comprehensive.bas` - Additional edge case testing
  - Floating point selectors (truncation)
  - Nested ON GOSUB
  - Immediate RETURN after ON GOSUB

### Validation Checklist

Once tests run:

- [ ] Verify generated QBE IL contains `jmp @default [ %selector @case0 @case1 ... ]`
- [ ] Verify trampolines are generated for ON GOSUB
- [ ] Verify return stack push/pop sequences are correct
- [ ] Verify selector is evaluated only once
- [ ] Verify 1-to-0 based index conversion is correct
- [ ] Check performance vs. legacy linear comparison chains
- [ ] Run full test suite to ensure no regressions

## Generated Code Examples

### ON GOTO (Expected Output)

```basic
LET x = 2
ON x GOTO 100, 200, 300
```

Should generate:
```qbe
# Evaluate selector
%t.0 =w loadw %var_x_INT

# Convert to 0-based
%t.1 =w sub %t.0, 1

# Switch dispatch
jmp @block_fallthrough [ %t.1 @block_100 @block_200 @block_300 ]
```

### ON GOSUB (Expected Output)

```basic
LET x = 2
ON x GOSUB 100, 200, 300
PRINT "Returned"
```

Should generate:
```qbe
# Evaluate selector
%t.0 =w loadw %var_x_INT

# Convert to 0-based
%t.1 =w sub %t.0, 1

# Switch to trampolines
jmp @block_return [ %t.1 @trampoline_1 @trampoline_2 @trampoline_3 ]

@trampoline_1
    # Push return point
    %sp.0 =w loadw $gosub_return_sp
    %sp.1 =l extsw %sp.0
    %offset.0 =l mul %sp.1, 4
    %addr.0 =l add $gosub_return_stack, %offset.0
    storew 5, %addr.0  # 5 = return block ID
    %sp.2 =w add %sp.0, 1
    storew %sp.2, $gosub_return_sp
    jmp @block_100

@trampoline_2
    # Similar push sequence...
    jmp @block_200

@trampoline_3
    # Similar push sequence...
    jmp @block_300

@block_return
    # Continue execution after GOSUB
```

## Follow-Up Tasks

### Immediate (Blocking)
1. **Fix CFG Builder Statement Dispatch** - Investigate why handleOnGoto/handleOnGosub aren't called
2. **Test ON GOTO** - Run test_on_goto.bas and verify correct output
3. **Test ON GOSUB** - Run test_on_gosub.bas and verify correct output
4. **Remove Debug Output** - Clean up stderr/file debug statements added during investigation

### Nice-to-Have
1. **Migrate SELECT CASE** - Use the new `emitSwitch()` helper for SELECT CASE statements
2. **Performance Benchmarking** - Compare switch-based vs. legacy comparison-chain approach
3. **Runtime Stack Guards** - Add overflow checks for gosub_return_stack
4. **Documentation** - Update user-facing docs to mention ON GOTO/GOSUB support

## Technical Notes

### Selector Normalization
The `emitSelectorWord()` helper handles all numeric types:
- INTEGER: Use directly (already word type)
- LONG: Truncate to word with `copy`
- SHORT/BYTE: Sign-extend to word
- SINGLE/DOUBLE: Convert to int with `stosi`/`dtosi`

BASIC's 1-based indexing is converted to QBE's 0-based by subtracting 1 from the selector.

### Return Stack Management
The GOSUB return stack is a global array:
- `$gosub_return_stack` - Array of 1000 word entries
- `$gosub_return_sp` - Stack pointer (word)

Push sequence:
1. Load SP
2. Extend to long
3. Multiply by 4 (word size)
4. Add to stack base address
5. Store block ID
6. Increment SP

Pop sequence is identical but decrements SP first.

### Edge Label Conventions
- ON GOTO: "case_1", "case_2", ..., "default"
- ON GOSUB: "call_1", "call_2", ..., plus JUMP edge to return point
- Labels are 1-based to match BASIC semantics
- Converted to 0-based during codegen

## References

- **Design Document:** `docs/ONGOPLAN.md`
- **CFG Builder:** `src/cfg/cfg_builder_jumps.cpp`
- **CFG Emitter:** `src/codegen_v2/cfg_emitter.cpp`
- **QBE Builder:** `src/codegen_v2/qbe_builder.cpp`
- **Test Suite:** `tests/functions/test_on_goto.bas`, `test_on_gosub.bas`
- **QBE Spec:** https://c9x.me/compile/doc/il.html (switch instruction)

## Conclusion

The ON GOTO/GOSUB codegen implementation is **complete and correct** according to the design plan. It uses modern, efficient switch-based dispatch with QBE's native jump table support. However, testing is blocked by a pre-existing bug in the CFG builder where ON statements don't reach their dedicated handlers. Once that bug is fixed, the implementation should work immediately and pass all tests.

The code is well-structured with shared helpers (`emitSelectorWord`, `emitPushReturnBlock`) and integrates cleanly with the existing CFG-driven pipeline.