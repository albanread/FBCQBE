# Exception Handling Phase 4 Complete: Semantic Analysis & CFG

## Overview

Phase 4 of the exception handling implementation has been completed. This phase adds semantic validation and control flow graph (CFG) support for TRY/CATCH/FINALLY/THROW statements.

**Status**: ✅ Complete  
**Date**: January 29, 2025  
**Phase**: 4 of 6 (Semantic Analysis & CFG)

---

## What Was Implemented

### 1. Semantic Validation (`fasterbasic_semantic.cpp`)

Added comprehensive validation for exception handling constructs:

#### `validateTryCatchStatement()`
- **Rule 1**: TRY must have at least one CATCH clause OR a FINALLY block
- **Rule 2**: Validates each CATCH clause:
  - Catch-all (empty error codes) must be the last CATCH clause
  - Error codes must be positive integers
  - No duplicate error codes within a CATCH
  - Validates all statements in CATCH block
- **Rule 3**: Validates all statements in TRY block
- **Rule 4**: Validates all statements in FINALLY block (if present)

#### `validateThrowStatement()`
- Validates THROW has an error code expression
- Error code must be numeric type (will be converted to integer)
- Warning if constant error code is <= 0
- Validates the error code expression

#### Integration
- Added cases in `validateStatement()` to dispatch to new validators
- Proper error reporting with source location tracking

### 2. Control Flow Graph Support (`fasterbasic_cfg.cpp`)

Implemented CFG construction for exception handling:

#### `processTryCatchStatement()`
Creates the complete exception handling structure:
```
TRY Setup Block (setjmp)
    ↓
TRY Body Block (executes TRY statements)
    ↓ (normal)          ↘ (exception)
FINALLY/Exit         Exception Dispatch Block
                         ↓ (match)
                     CATCH Blocks (one per clause)
                         ↓
                     FINALLY Block (if present)
                         ↓
                     Exit Block
```

**Blocks Created**:
1. **TRY Setup Block**: Contains exception context setup (will emit setjmp)
2. **TRY Body Block**: Executes TRY statements
3. **Exception Dispatch Block**: Routes exceptions to appropriate CATCH
4. **CATCH Blocks**: One per CATCH clause (labeled with error codes)
5. **FINALLY Block**: Cleanup code (optional)
6. **Exit Block**: Continuation after END TRY

#### `buildEdges()` Exception Handling
Added edge creation logic for exception control flow:

**TRY Setup Block Edges**:
- Unconditional edge → TRY Body Block

**TRY Body Block Edges**:
- Normal completion: Fallthrough → FINALLY (or Exit if no FINALLY)
- Exception path: Conditional edge → Exception Dispatch Block

**Exception Dispatch Block Edges**:
- Conditional edges → Each CATCH block (based on error code match)
- Implicit re-throw if no match (handled by runtime)

**CATCH Block Edges**:
- Fallthrough → FINALLY (or Exit if no FINALLY)

**FINALLY Block Edges**:
- Fallthrough → Exit Block

**THROW Statement**:
- Marked as terminator (doesn't return normally)
- Exception routing handled by setjmp/longjmp at runtime

### 3. Data Structure Updates

#### `fasterbasic_cfg.h` - ControlFlowGraph
Added `TryCatchBlocks` structure:
```cpp
struct TryCatchBlocks {
    int tryBlock;              // Block that sets up exception context (setjmp)
    int tryBodyBlock;          // First block of TRY body statements
    int dispatchBlock;         // Block that dispatches to appropriate CATCH
    std::vector<int> catchBlocks;  // One block per CATCH clause
    int finallyBlock;          // FINALLY block (-1 if none)
    int exitBlock;             // Block after END TRY
    bool hasFinally;           // Whether FINALLY is present
    const TryCatchStatement* tryStatement;  // Pointer to AST node
};
std::map<int, TryCatchBlocks> tryCatchStructure;  // tryBlock → TryCatchBlocks
```

#### `fasterbasic_cfg.h` - CFGBuilder
Added `TryCatchContext` for tracking during CFG construction:
```cpp
struct TryCatchContext {
    int tryBlock;              // Block that sets up exception context
    int tryBodyBlock;          // First block of TRY body
    int dispatchBlock;         // Exception dispatcher block
    std::vector<int> catchBlocks;  // CATCH handler blocks
    int finallyBlock;          // FINALLY block (-1 if none)
    int exitBlock;             // Block after END TRY
    bool hasFinally;           // Whether FINALLY is present
    const TryCatchStatement* tryStatement;
};
std::vector<TryCatchContext> m_tryCatchStack;
```

### 4. Method Declarations

#### `fasterbasic_semantic.h`
```cpp
// Exception handling statement validation
void validateTryCatchStatement(const TryCatchStatement& stmt);
void validateThrowStatement(const ThrowStatement& stmt);
```

#### `fasterbasic_cfg.h`
```cpp
void processTryCatchStatement(const TryCatchStatement& stmt, BasicBlock* currentBlock);
```

---

## Compilation Status

✅ **Both modified files compile successfully**:
- `fasterbasic_semantic.cpp` → Compiles with warnings (unrelated to changes)
- `fasterbasic_cfg.cpp` → Compiles with warnings (unrelated to changes)

Warnings are pre-existing and related to incomplete switch statements for other enums.

---

## Control Flow Examples

### Example 1: Simple TRY/CATCH
```basic
TRY
    x% = 10 / 0
CATCH 11
    PRINT "Division by zero!"
END TRY
```

**CFG Structure**:
```
Block 0: TRY Setup (setjmp)
    → Block 1

Block 1: TRY Body
    STMT: x% = 10 / 0
    → Block 3 (normal)
    → Block 2 (exception)

Block 2: Exception Dispatch
    Check ERR() == 11
    → Block 3 (if match)
    → re-throw (if no match)

Block 3: CATCH 11
    STMT: PRINT "Division by zero!"
    → Block 4

Block 4: Exit (after END TRY)
```

### Example 2: TRY/CATCH/FINALLY
```basic
TRY
    OPEN "data.txt" FOR INPUT AS #1
    READ_DATA(1)
CATCH 53
    PRINT "File not found"
FINALLY
    CLOSE #1
END TRY
```

**CFG Structure**:
```
Block 0: TRY Setup
    → Block 1

Block 1: TRY Body
    STMT: OPEN "data.txt" FOR INPUT AS #1
    STMT: CALL READ_DATA(1)
    → Block 4 (normal)
    → Block 2 (exception)

Block 2: Exception Dispatch
    Check ERR() == 53
    → Block 3 (if match)
    → re-throw (if no match)

Block 3: CATCH 53
    STMT: PRINT "File not found"
    → Block 4

Block 4: FINALLY
    STMT: CLOSE #1
    → Block 5

Block 5: Exit
```

### Example 3: Multiple CATCH Clauses
```basic
TRY
    PROCESS_FILE()
CATCH 9, 11
    PRINT "Math error"
CATCH 53
    PRINT "File error"
CATCH
    PRINT "Unknown error"
END TRY
```

**CFG Structure**:
```
Block 0: TRY Setup
    → Block 1

Block 1: TRY Body
    STMT: CALL PROCESS_FILE()
    → Block 5 (normal)
    → Block 2 (exception)

Block 2: Exception Dispatch
    → Block 3 (if ERR() == 9 or 11)
    → Block 4 (if ERR() == 53)
    → Block 5 (catch-all)

Block 3: CATCH 9, 11
    STMT: PRINT "Math error"
    → Block 6

Block 4: CATCH 53
    STMT: PRINT "File error"
    → Block 6

Block 5: CATCH (all)
    STMT: PRINT "Unknown error"
    → Block 6

Block 6: Exit
```

---

## Validation Rules Enforced

### TRY/CATCH Statement Rules
1. ✅ Must have at least one CATCH or FINALLY
2. ✅ Error codes must be positive integers
3. ✅ No duplicate error codes in a CATCH clause
4. ✅ Catch-all CATCH must be last
5. ✅ All nested statements validated recursively

### THROW Statement Rules
1. ✅ Must have an error code expression
2. ✅ Error code must be numeric
3. ✅ Warning if constant code <= 0
4. ✅ Expression validated for type correctness

### CFG Construction Rules
1. ✅ All paths through TRY go to FINALLY (if present)
2. ✅ All CATCH handlers go to FINALLY (if present)
3. ✅ FINALLY always goes to Exit
4. ✅ Exception path from TRY to Dispatch properly modeled
5. ✅ Each CATCH gets its own basic block

---

## Testing Recommendations

### Unit Tests Needed (Phase 6)
1. **Valid TRY/CATCH structures**:
   - Single CATCH with one code
   - Multiple CATCH clauses
   - CATCH with multiple codes (9, 11)
   - Catch-all CATCH
   - TRY/FINALLY (no CATCH)
   - TRY/CATCH/FINALLY

2. **Invalid structures (should error)**:
   - TRY without CATCH or FINALLY
   - Catch-all not last
   - Duplicate error codes
   - Negative error codes
   - THROW with no expression
   - THROW with string expression

3. **Nested TRY blocks**:
   - TRY inside TRY
   - TRY inside CATCH
   - TRY inside FINALLY

4. **CFG validation**:
   - Verify block structure
   - Verify edge connectivity
   - Verify terminator blocks marked correctly

---

## What's Next: Phase 5 (Code Generation)

The next phase will implement QBE IL generation for exception handling:

### Tasks Remaining
1. **Emit TRY Setup**:
   - Call `basic_exception_push(has_finally)`
   - Emit setjmp wrapper and store result
   - Branch based on setjmp return value

2. **Emit TRY Body**:
   - Generate code for TRY statements
   - Any runtime error calls `basic_throw(code)`

3. **Emit Exception Dispatch**:
   - Call `basic_err()` to get error code
   - Compare against each CATCH clause's codes
   - Jump to matching CATCH or re-throw

4. **Emit CATCH Blocks**:
   - Generate code for each CATCH body
   - Jump to FINALLY or Exit after completion

5. **Emit FINALLY Block**:
   - Generate cleanup code
   - Always runs regardless of exception

6. **Emit THROW Statement**:
   - Evaluate error code expression
   - Convert to integer if needed
   - Call `basic_throw(code)`

7. **Wire ERR/ERL Functions**:
   - Emit calls to `basic_err()` and `basic_erl()`
   - Return current error code and line number

### Estimated Effort
- **Time**: 3-5 days
- **Complexity**: High (setjmp/longjmp integration, correct QBE types)
- **Risk Areas**: 
  - Correct register allocation across setjmp
  - FINALLY execution on all paths
  - Nested exception context management

---

## Files Modified

### Modified Files
1. `fsh/FasterBASICT/src/fasterbasic_semantic.h`
   - Added validation method declarations

2. `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`
   - Implemented `validateTryCatchStatement()`
   - Implemented `validateThrowStatement()`
   - Added cases to `validateStatement()` switch

3. `fsh/FasterBASICT/src/fasterbasic_cfg.h`
   - Added `TryCatchBlocks` structure to `ControlFlowGraph`
   - Added `TryCatchContext` to `CFGBuilder`
   - Added `processTryCatchStatement()` declaration

4. `fsh/FasterBASICT/src/fasterbasic_cfg.cpp`
   - Implemented `processTryCatchStatement()`
   - Added TRY/CATCH cases to `processStatement()` switch
   - Added TRY/CATCH edge logic to `buildEdges()`
   - Added THROW terminator handling

### No Changes Required
- Lexer (already done in Phase 2)
- Parser (already done in Phase 3)
- Runtime (already done in Phase 1)
- AST (already done in Phase 3)

---

## Summary

Phase 4 successfully adds semantic validation and CFG support for exception handling. The compiler can now:

1. ✅ Validate TRY/CATCH/FINALLY syntax and semantics
2. ✅ Validate THROW statements
3. ✅ Build control flow graphs for exception constructs
4. ✅ Track exception context through nested structures
5. ✅ Model normal and exceptional control flow paths
6. ✅ Prepare for code generation in Phase 5

The implementation follows the same patterns used for other control flow constructs (SELECT CASE, FOR/WHILE loops) and integrates cleanly with the existing semantic analyzer and CFG builder.

**Next Step**: Proceed to Phase 5 (Code Generation) to emit QBE IL for exception handling.