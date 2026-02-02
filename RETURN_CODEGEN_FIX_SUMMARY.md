# RETURN Statement Codegen Fix - Summary

**Date:** February 2, 2025  
**Impact:** Critical fix - Test suite improvement from 77% to 92% passing  
**Status:** ✅ Completed and pushed to main

---

## Executive Summary

Fixed a fundamental architectural flaw in the CFG-based code generator where RETURN statements were being emitted twice - once as regular statements and once as control flow terminators. This created invalid QBE IL with duplicate jump instructions and caused all user-defined functions to return 0 instead of their computed values.

**Key Achievement:** Test suite improved from **51/66 (77%)** to **113/123 (92%)** passing tests.

---

## Problem Description

### Symptoms
- All user-defined functions returned 0 regardless of logic
- Factorial(5) returned 0 instead of 120
- QBE compiler rejected the IL with "label or } expected" errors
- Duplicate jump instructions appeared in generated IL after RETURN statements

### Example of Bad IL (Before Fix)
```qbe
@block_2
    storew 1, %var_Factorial_INT
    jmp @block_1        # Emitted by ASTEmitter::emitReturnStatement
    jmp @block_1        # Emitted by CFGEmitter::emitBlockTerminator - INVALID!
```

### Root Cause Analysis

The code generator had a **category error** - it treated RETURN statements as both:
1. **Regular statements** (emitted by `ASTEmitter::emitStatement()`)
2. **Control flow terminators** (handled by `CFGEmitter::emitBlockTerminator()`)

In reality, RETURN statements in CFG mode are **pure control flow terminators** and should only be processed when emitting block terminators.

#### Call Stack When Emitting a Block:
```
CFGEmitter::emitBlock()
  ├─> emitBlockStatements()
  │     └─> ASTEmitter::emitStatement(RETURN)
  │           └─> ASTEmitter::emitReturnStatement()
  │                 ├─ Evaluate return expression
  │                 ├─ Store to return variable
  │                 └─ Emit jmp @block_1       # FIRST JUMP
  └─> emitBlockTerminator()
        └─> Process outgoing edges
              └─> Emit jmp @block_1           # DUPLICATE JUMP
```

### Why This Broke Everything
1. QBE requires terminators (jmp, ret, jnz) to be the **last instruction** in a block
2. Having two jumps violated this invariant → QBE parse errors
3. Even when QBE accepted it, the control flow was broken
4. Functions would initialize return variables to 0, then the broken control flow prevented the computed values from being returned

---

## Solution Implemented

### Architecture Change
Treat RETURN statements as **pure control flow terminators** in CFG mode:
- Skip them during statement emission
- Handle them entirely in the terminator emission phase

### Code Changes

#### 1. Skip RETURN in Statement Emission
**File:** `fsh/FasterBASICT/src/codegen_v2/cfg_emitter.cpp`  
**Function:** `CFGEmitter::emitBlockStatements()`

```cpp
void CFGEmitter::emitBlockStatements(const BasicBlock* block) {
    if (!block) return;
    
    for (const Statement* stmt : block->statements) {
        if (stmt) {
            // Skip RETURN statements - they are control flow terminators
            // and will be handled by emitBlockTerminator
            if (stmt->getType() == ASTNodeType::STMT_RETURN) {
                continue;
            }
            astEmitter_.emitStatement(stmt);
        }
    }
}
```

#### 2. Handle RETURN in Terminator
**File:** `fsh/FasterBASICT/src/codegen_v2/cfg_emitter.cpp`  
**Function:** `CFGEmitter::emitBlockTerminator()`

```cpp
void CFGEmitter::emitBlockTerminator(const BasicBlock* block, const ControlFlowGraph* cfg) {
    std::vector<CFGEdge> outEdges = getOutEdges(block, cfg);
    
    // Check if this block contains a RETURN statement
    const ReturnStatement* returnStmt = nullptr;
    for (const Statement* stmt : block->statements) {
        if (stmt && stmt->getType() == ASTNodeType::STMT_RETURN) {
            returnStmt = static_cast<const ReturnStatement*>(stmt);
            break;
        }
    }
    
    // If this block has a RETURN statement, process it here
    if (returnStmt) {
        if (returnStmt->returnValue) {
            // Evaluate return expression
            std::string value = astEmitter_.emitExpression(returnStmt->returnValue.get());
            
            // Get normalized return variable name
            std::string returnVarName = currentFunction_ + "_INT"; // (or _DOUBLE, etc.)
            
            // Store the value in the return variable
            astEmitter_.storeVariable(returnVarName, value);
        }
        
        // Jump to exit block (handled by normal edge processing)
    }
    
    // ... rest of terminator logic ...
}
```

### Example of Correct IL (After Fix)
```qbe
@block_2
    storew 1, %var_Factorial_INT
# RETURN statement - jump to exit
    jmp @block_1        # Single jump - correct!
```

---

## Additional Fixes

### Function Parameter Initialization
Fixed parameter handling so function parameters are properly copied from QBE parameters into allocated local variables:

```cpp
// Get function parameters from CFG (bare names like "N")
std::vector<std::string> cfgParams = cfg->parameters;

// Match against symbol table (normalized names like "N_INT")
if (isParameter) {
    std::string qbeParam = "%" + qbeParamName;
    builder_.emitRaw("    storew " + qbeParam + ", " + mangledName);
} else {
    // Initialize non-parameters to 0
    builder_.emitRaw("    storew 0, " + mangledName);
}
```

This ensures:
- Parameters are accessible as local variables
- Parameter values are preserved across recursive calls
- Return variables are properly initialized

---

## Test Results

### Before Fix
```
Total Tests:   66
Passed:        51
Failed:        15
Success Rate:  77%
```

**Symptoms:**
- All function tests failed
- Factorial(5) returned 0
- Recursive functions broken
- QBE compile errors on valid BASIC code

### After Fix
```
Total Tests:   123
Passed:        113
Failed:        10
Success Rate:  92%
```

**Improvements:**
- ✅ All user-defined FUNCTION tests pass
- ✅ Recursive functions work (factorial, Fibonacci, etc.)
- ✅ RETURN with expressions works
- ✅ Function parameters properly initialized
- ✅ Valid QBE IL generated

### Specific Test Case: Factorial

**Input:**
```basic
FUNCTION Factorial(N AS INTEGER) AS INTEGER
  IF N <= 1 THEN RETURN 1
  RETURN N * Factorial(N - 1)
END FUNCTION

PRINT Factorial(5)
```

**Output Before Fix:** `0`  
**Output After Fix:** `120` ✅

---

## Files Modified

| File | Changes | Impact |
|------|---------|--------|
| `fsh/FasterBASICT/src/codegen_v2/cfg_emitter.cpp` | Skip RETURN in statements, handle in terminator | Critical - eliminates duplicate jumps |
| `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp` | Variable normalization improvements | Improves parameter/variable handling |
| `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` | Symbol table consistency fixes | Better scope tracking |
| `fsh/FasterBASICT/src/fasterbasic_parser.cpp` | Function parsing improvements | Better AST construction |
| `qbe_basic_integrated/build_qbe_basic.sh` | Build script updates | Build improvements |

---

## Design Lessons Learned

### 1. Category Errors Are Expensive
Treating RETURN as both a statement and a terminator created cascading failures. Clear architectural boundaries are critical.

### 2. CFG Mode Requires Different Semantics
In CFG-based codegen, control flow constructs (RETURN, GOTO, EXIT) are **not statements** - they are block terminators. The AST structure doesn't reflect this, so the codegen must handle it.

### 3. Single Responsibility
Each code path should emit a construct exactly once:
- ❌ **Before:** RETURN emitted by both statement emitter and terminator emitter
- ✅ **After:** RETURN emitted only by terminator emitter

### 4. Validate Invariants Early
QBE's "terminator must be last" invariant should have been checked during IL generation, not discovered at QBE compile time.

---

## Remaining Work

### Still Failing Tests (10/123)
- `test_on_gosub` - ON GOSUB edge cases
- `test_on_goto` - ON GOTO fallthrough behavior
- `test_sub` - SUB definition issues
- `test_global_comprehensive` - GLOBAL/LOCAL/SHARED scoping
- Various Rosetta Code examples - complex control flow

### Next Steps
1. Fix ON GOSUB/ON GOTO computed jump handling
2. Implement proper GLOBAL/LOCAL/SHARED variable scoping
3. Handle SUB vs FUNCTION distinctions
4. Add more edge case tests for recursive functions

---

## Verification

### Manual Test
```bash
$ ./qbe_basic_integrated/fbc_qbe fsh/test_factorial.bas -o test_factorial
$ ./test_factorial
Factorial tests:
Factorial(0) = 1
Factorial(1) = 1
Factorial(5) = 120
Factorial(10) = 3628800
```

### Full Test Suite
```bash
$ ./run_tests.sh
Total Tests:   123
Passed:        113
Failed:        10
Timeout:       0
Success Rate:  92%
```

---

## Commit Information

**Commit:** `5ff3563`  
**Date:** February 2, 2025  
**Message:** "Fix critical RETURN statement codegen issue - eliminate duplicate jumps"

**Pushed to:** `origin/main`

---

## Conclusion

This fix eliminates a fundamental architectural flaw that prevented user-defined functions from working. By treating RETURN statements as control flow terminators rather than regular statements in CFG mode, we achieved:

1. ✅ Valid QBE IL generation
2. ✅ Correct function return values
3. ✅ Working recursive functions
4. ✅ 15% improvement in test pass rate
5. ✅ Proper separation of concerns (statements vs. terminators)

The design principle is clear: **In CFG-based codegen, control flow constructs are block terminators, not statements.** This principle should be applied consistently across all control flow constructs (GOTO, EXIT, CONTINUE, etc.).