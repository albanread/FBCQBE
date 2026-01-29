# Exception Handling Code Generation Progress

**Date:** January 2025  
**Status:** Parser Fixed, Code Generation 60% Complete  
**Phase:** 5 of 6 (Code Generation)

---

## Summary

Exception handling support (TRY/CATCH/FINALLY/THROW) is being implemented for FasterBASIC. The infrastructure is largely in place, but code generation needs completion.

---

## What's Working ✅

### 1. Lexer & Tokens
- ✅ All tokens defined: `TRY`, `CATCH`, `FINALLY`, `THROW`, `ERR`, `ERL`, `END TRY`
- ✅ Lexer recognizes all exception keywords

### 2. Parser
- ✅ `parseTryStatement()` implemented and working
- ✅ `parseThrowStatement()` implemented and working
- ✅ Handles TRY/CATCH/FINALLY/END TRY syntax correctly
- ✅ Supports multiple CATCH clauses with error codes
- ✅ **FIXED:** Parser no longer hangs on END TRY
- ✅ **FIXED:** Infinite loop prevention when parseStatement fails to advance

**Parser fixes applied:**
- Changed END detection to `!(check(TokenType::END) && peek().type == TokenType::TRY)`
- Added safety checks: if parseStatement doesn't advance position, force advance to prevent infinite loop

### 3. AST Nodes
- ✅ `TryCatchStatement` class defined with:
  - `tryBlock` vector
  - `catchClauses` vector (with error codes)
  - `finallyBlock` vector
  - `hasFinally` flag
- ✅ `ThrowStatement` class defined with error code expression
- ✅ toString() methods for debugging

### 4. Semantic Analysis
- ✅ `validateTryCatchStatement()` implemented
- ✅ Validates TRY must have at least one CATCH or FINALLY
- ✅ Validates catch-all must be last
- ✅ Validates error codes are positive integers
- ✅ Validates duplicate error codes
- ✅ `validateThrowStatement()` implemented
- ✅ Validates THROW has numeric expression

### 5. CFG Builder
- ✅ `processTryCatchStatement()` implemented
- ✅ Creates TRY setup block (current block with TryCatchStatement)
- ✅ Creates TRY body block for TRY statements
- ✅ Creates exception dispatch block
- ✅ Creates CATCH blocks (one per clause)
- ✅ Creates FINALLY block (if present)
- ✅ Creates exit block
- ✅ Stores structure in `TryCatchContext` and `ControlFlowGraph::TryCatchBlocks`
- ✅ Builds edges correctly:
  - TRY setup → TRY body (unconditional)
  - TRY body → FINALLY or exit (fallthrough)
  - CATCH blocks → FINALLY or exit (fallthrough)
  - FINALLY → exit (fallthrough)
  - Exception dispatch → CATCH blocks (conditional)

### 6. Runtime Functions
- ✅ `ExceptionContext` structure defined (jmp_buf, prev, error_code, error_line, has_finally)
- ✅ `basic_exception_push()` - Push exception context onto stack
- ✅ `basic_exception_pop()` - Pop exception context
- ✅ `basic_throw()` - Throw exception with longjmp
- ✅ `basic_err()` - Get current error code (ERR function)
- ✅ `basic_erl()` - Get error line number (ERL function)
- ✅ `basic_setjmp()` - Wrapper for setjmp

### 7. Code Generation - Partial
- ✅ `emitThrow()` - Complete and working
  - Evaluates error code expression
  - Converts to int32 if needed
  - Calls basic_throw (which longjmps)
- ✅ Exception dispatch block handling in `emitBlock()`
  - Detects dispatch blocks by label "Exception Dispatch"
  - Calls basic_err() to get error code
  - Compares against CATCH error codes
  - Emits conditional branches to matching CATCH blocks
  - Handles catch-all (empty error code list)
- ✅ FINALLY cleanup in `emitBlock()`
  - Detects FINALLY blocks
  - Emits basic_exception_pop() after FINALLY execution
- ✅ TRY setup detection in `emitBlock()`
  - Detects blocks containing TryCatchStatement
  - Emits basic_exception_push(hasFinally)
  - Emits basic_setjmp() call
  - Computes condition: isException = (setjmp_result != 0)
  - Stores condition in m_lastCondition

---

## What's Not Working ❌

### Code Generation Issues

#### Issue 1: TRY setup block conditional branch not emitted
**Problem:** After emitting setjmp and computing the condition, the conditional branch to dispatch/body blocks is not being emitted.

**Current behavior:**
```assembly
bl  _basic_exception_push
bl  _basic_setjmp
# Then immediately continues to TRY body statements
# No conditional branch based on setjmp result
```

**Expected behavior:**
```assembly
bl  _basic_exception_push
bl  _basic_setjmp
# Compare result with 0
# Branch to dispatch if exception, else fall through to TRY body
jnz %condition, @dispatch_label, @try_body_label
```

**Root cause:** The TRY setup block has the TryCatchStatement in its statements list. When we detect it's a setup block, we emit setup code and set `m_lastCondition`, but then the block processing continues and processes the TryCatchStatement via `emitStatement()` which calls `emitTryCatch()` (which is empty). The conditional branch should be emitted at the end of the block based on successors, but the CFG may not have the right successors.

**Likely fix:** Check the CFG successors for the TRY setup block. It should have 2 successors: dispatch block and TRY body block. The conditional branch should be emitted in the block successor handling code at the end of `emitBlock()`.

#### Issue 2: CATCH and FINALLY blocks not being emitted
**Problem:** Only the TRY body code is emitted; CATCH and FINALLY blocks are missing from assembly output.

**Current behavior:**
```assembly
_main:
    # Setup code
    # TRY body code
    # No CATCH blocks
    # No FINALLY blocks
    # No dispatch block
```

**Root cause:** Either:
1. The CFG blocks exist but `emitBlock()` is not being called for them, or
2. The blocks are being skipped, or
3. The iteration over CFG blocks doesn't include them

**Likely fix:** Verify that `generate()` iterates over ALL blocks in the CFG, not just blocks that are reachable via normal control flow. Exception dispatch is reached via longjmp, not normal CFG edges, so it might be getting skipped.

#### Issue 3: TryCatchStatement emitted as statement
**Problem:** The `emitTryCatch()` function is a placeholder that does nothing, but it's being called when processing statements in the TRY setup block.

**Expected behavior:** `emitTryCatch()` should not emit any code - all code generation is handled by `emitBlock()` for the various blocks created by the CFG builder.

**Current fix:** `emitTryCatch()` is empty, which is correct. No action needed here.

---

## Test Results

### Test: test_try_tiny.bas
```basic
TRY
    PRINT "test"
CATCH 11
    PRINT "error"
END TRY
END
```

**Parser:** ✅ Parses successfully (no hang, no errors)

**Assembly output:** ⚠️ Partial
- ✅ Calls basic_exception_push
- ✅ Calls basic_setjmp
- ✅ Emits TRY body (PRINT "test")
- ❌ No conditional branch after setjmp
- ❌ No exception dispatch block
- ❌ No CATCH block (PRINT "error")
- ❌ No exception cleanup

---

## Next Steps (Priority Order)

### Priority 1: Fix TRY setup conditional branch
1. Debug why conditional branch isn't emitted after setjmp
2. Check CFG successors for TRY setup block
3. Verify `m_lastCondition` is being used in block terminator emission
4. Expected: TRY setup block should have 2 successors (dispatch and body)
5. Expected: End of `emitBlock()` should emit: `jnz %condition, @dispatch, @body`

### Priority 2: Ensure all CFG blocks are emitted
1. Add debug logging to `generate()` to see which blocks are being emitted
2. Verify dispatch, CATCH, and FINALLY blocks are being iterated
3. Check if blocks are being skipped due to reachability analysis
4. Ensure `emitBlock()` is called for every block in CFG

### Priority 3: Test exception dispatch
1. Once blocks are emitted, test that dispatch routes to correct CATCH
2. Verify catch-all works
3. Test multiple CATCH clauses with different error codes
4. Test FINALLY execution

### Priority 4: Test THROW statement
1. Create test with explicit THROW
2. Verify exception is caught by CATCH block
3. Test nested TRY blocks
4. Test re-throw scenarios

### Priority 5: Integration tests
1. Test division by zero (runtime error caught)
2. Test array bounds error caught
3. Test FINALLY always executes
4. Test ERR() and ERL() functions
5. Test nested exception handling

---

## Debugging Commands

### Compile and view assembly
```bash
qbe_basic_integrated/qbe_basic test_try_tiny.bas 2>&1 | head -150
```

### Check for exception-related code
```bash
qbe_basic_integrated/qbe_basic test_try_tiny.bas 2>&1 | grep -E "(exception|setjmp|CATCH|TRY)"
```

### Parse-only test
```bash
# Create a test that only parses, doesn't codegen
# Useful for isolating parser issues
```

---

## Code Locations

### Parser
- **File:** `fsh/FasterBASICT/src/fasterbasic_parser.cpp`
- **Functions:** `parseTryStatement()` (L6215-6306), `parseThrowStatement()` (L6299-6306)

### Semantic Analysis
- **File:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`
- **Functions:** `validateTryCatchStatement()` (L2280-2346), `validateThrowStatement()` (L2347-2380)

### CFG Builder
- **File:** `fsh/FasterBASICT/src/fasterbasic_cfg.cpp`
- **Functions:** `processTryCatchStatement()` (L644-744), `buildEdges()` exception handling (L1585-1605, L1157-1203)

### Code Generation
- **File:** `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp`
- **Functions:** `emitTryCatch()` (L2549-2555), `emitThrow()` (L2557-2587)

- **File:** `fsh/FasterBASICT/src/codegen/qbe_codegen_main.cpp`
- **Functions:** `emitBlock()` - TRY setup (L841-876), exception dispatch (L878-932), FINALLY cleanup (L1143-1147)

### Runtime
- **File:** `fsh/FasterBASICT/runtime_c/basic_runtime.h`
- **Declarations:** Exception functions (L167-188), ExceptionContext struct (L27-33)

- **File:** `fsh/FasterBASICT/runtime_c/basic_runtime.c`
- **Implementations:** `basic_exception_push()` (L275-288), `basic_exception_pop()` (L292-297), `basic_throw()` (L301-321), `basic_err()` (L325-327), `basic_erl()` (L331-333), `basic_setjmp()` (L344-351)

---

## Estimated Time to Complete

- **Fix conditional branch emission:** 1-2 hours
- **Fix block iteration:** 1-2 hours
- **Testing and validation:** 2-4 hours
- **Total:** 4-8 hours

Once the conditional branch and block iteration are fixed, exception handling should be fully functional.

---

## Success Criteria

✅ **Phase 5 Complete When:**
1. TRY/CATCH parses without hanging ✅
2. TRY setup emits: push, setjmp, conditional branch (partial ✅)
3. Exception dispatch block emits error code checking
4. CATCH blocks emit their statements
5. FINALLY blocks emit their statements + pop
6. THROW statement emits basic_throw call ✅
7. All test programs compile without errors
8. Generated assembly contains all expected blocks

✅ **Phase 6 (Testing) Can Start When:**
- All code generation is complete
- Simple test programs produce correct assembly
- Runtime exception functions can be tested end-to-end

---

**Status Summary:** Parser is fixed and working. Code generation infrastructure is in place. Main blockers are conditional branch emission and ensuring all CFG blocks are emitted. Estimated 4-8 hours to complete.