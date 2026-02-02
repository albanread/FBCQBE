# Session Summary: SUB Parameter Fix
**Date:** February 2, 2025  
**Focus:** Fix SUB statement parameter handling  
**Result:** 113/123 → 115/123 tests passing (93.5%)

---

## Problem Discovery

### Initial Investigation
Started by examining the CFG (Control Flow Graph) for the factorial test to understand the code generation pipeline. Used `-G` flag to display CFG structure:

```bash
./qbe_basic_integrated/fbc_qbe -G fsh/test_factorial.bas
```

This revealed a well-formed CFG with proper block structure and edges.

### The Real Issue
After fixing RETURN statement handling, discovered `test_sub.bas` was still failing with:
```
qbe:tests/functions/test_sub.bas:233: invalid type for first operand %t.1 in arg
```

### Root Cause Analysis

Examining the generated IL revealed:
```qbe
export function $sub_PrintInfo(l %label_STRING, w %num_INT, d %value_DOUBLE) {
@block_0
    %t.0 =l call $string_new_utf8(l $str_15)
    call $basic_print_string_desc(l %t.0)
# ERROR: variable not found: label_STRING (normalized: label_STRING)
    call $basic_print_string_desc(l %t.1)  # %t.1 was never assigned!
```

The error message in the IL showed: **"ERROR: variable not found: label_STRING"**

### Symbol Table Investigation
```bash
./qbe_basic_integrated/fbc_qbe -S tests/functions/test_sub.bas
```

**Before Fix:**
```
Variables (0):

Functions (7):
  PrintInfo: returnTypeDesc=VOID
  PrintMessage: returnTypeDesc=VOID
  ...
```

**Zero variables!** The SUB parameters weren't in the symbol table.

---

## Root Cause

### Architectural Inconsistency
Compared `processSubStatement()` with `processDefStatement()`:

**DEF FN (working):**
```cpp
void SemanticAnalyzer::processDefStatement(const DefStatement& stmt) {
    // ... process function ...
    
    for (size_t i = 0; i < stmt.parameters.size(); ++i) {
        // ... determine type ...
        
        // ✅ Add parameter as a variable in the symbol table
        Scope funcScope = Scope::makeFunction(stmt.functionName);
        VariableSymbol paramVar(paramName, paramTypeDesc, funcScope, true);
        paramVar.firstUse = stmt.location;
        m_symbolTable.insertVariable(paramName, paramVar);
    }
}
```

**SUB (broken):**
```cpp
void SemanticAnalyzer::processSubStatement(const SubStatement& stmt) {
    // ... process subroutine ...
    
    for (size_t i = 0; i < stmt.parameters.size(); ++i) {
        // ... determine type ...
        
        sym.parameterTypeDescs.push_back(legacyTypeToDescriptor(paramType));
        
        // ❌ Missing: Add to symbol table!
    }
    
    m_symbolTable.functions[stmt.subName] = sym;
}
```

**The Bug:** `processSubStatement()` stored parameters in `FunctionSymbol.parameters` but did NOT add them to `m_symbolTable.variables`.

### Why This Broke
1. Parser creates SUB statement with parameters: `SUB PrintInfo(label$, num%, value#)`
2. Semantic analyzer processes SUB:
   - Stores parameters in function metadata ✓
   - **FAILS** to add them as variables ✗
3. Codegen tries to reference `label_STRING` in SUB body
4. Variable lookup fails → emits `%t.1` without assignment
5. QBE rejects IL: "invalid type for first operand"

---

## Solution

### Code Fix
**File:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`  
**Function:** `processSubStatement()`

```cpp
void SemanticAnalyzer::processSubStatement(const SubStatement& stmt) {
    // ... existing code ...
    
    for (size_t i = 0; i < stmt.parameters.size(); ++i) {
        // ... determine paramType ...
        
        sym.parameterTypeDescs.push_back(legacyTypeToDescriptor(paramType));
        
        // ✅ FIX: Add parameter as a variable in the symbol table
        Scope funcScope = Scope::makeFunction(stmt.subName);
        TypeDescriptor paramTypeDesc = legacyTypeToDescriptor(paramType);
        VariableSymbol paramVar(stmt.parameters[i], paramTypeDesc, funcScope, true);
        paramVar.firstUse = stmt.location;
        m_symbolTable.insertVariable(stmt.parameters[i], paramVar);
    }
    
    m_symbolTable.functions[stmt.subName] = sym;
}
```

### What Changed
1. Added parameter insertion into `m_symbolTable.variables`
2. Each parameter gets proper function scope: `function:PrintInfo::label_STRING`
3. Parameters are marked as declared and ready for use
4. Matches the pattern already used for DEF FN statements

---

## Verification

### Symbol Table (After Fix)
```
Variables (6):
  function:PrintInfo::num_INT: typeDesc=INTEGER (isDeclared=1, isUsed=0)
  function:PrintMessage::msg_STRING: typeDesc=STRING (isDeclared=1, isUsed=0)
  function:PrintInfo::value_DOUBLE: typeDesc=DOUBLE (isDeclared=1, isUsed=0)
  function:PrintSum::b_INT: typeDesc=INTEGER (isDeclared=1, isUsed=0)
  function:PrintInfo::label_STRING: typeDesc=STRING (isDeclared=1, isUsed=0)
  function:PrintSum::a_INT: typeDesc=INTEGER (isDeclared=1, isUsed=0)

Functions (7):
  PrintInfo: returnTypeDesc=VOID
  ...
```

### Generated IL (After Fix)
```qbe
export function $sub_PrintInfo(l %label_STRING, w %num_INT, d %value_DOUBLE) {
@block_0
    %var_num_INT =l alloc4 4
    storew %num_INT, %var_num_INT
    %var_value_DOUBLE =l alloc8 8
    stored %value_DOUBLE, %var_value_DOUBLE
    %var_label_STRING =l alloc8 8
    storel %label_STRING, %var_label_STRING
    %t.0 =l call $string_new_utf8(l $str_15)
    call $basic_print_string_desc(l %t.0)
# Loading parameter: label_STRING
    call $basic_print_string_desc(l %label_STRING)  # ✅ Valid!
```

### Test Results
```bash
$ ./qbe_basic_integrated/fbc_qbe tests/functions/test_sub.bas -o test_sub
$ ./test_sub
SUB Implementation Test
=======================

Test 1: SUB with no parameters (explicit CALL)
  Hello from SUB

Test 3: SUB with parameters (explicit CALL)
  Sum: 8

Test 5: SUB with string parameter
  Message: Hello from SUB!

Test 6: SUB with mixed parameter types
  Result: num=42, value=3.14

Test 7: SUB calling another SUB
  CallOtherSub calling PrintHello
  Hello from SUB

Test 8: Nested SUB calls
  OuterSub start
    InnerSub executed
  OuterSub end

All SUB tests passed!
```

---

## Impact

### Test Suite Progress
- **Before:** 113/123 passing (92.0%)
- **After:** 115/123 passing (93.5%)
- **Fixed:** test_sub.bas, test_global_comprehensive.bas

### What Now Works
- ✅ SUB with no parameters
- ✅ SUB with INTEGER parameters (`a%`, `b%`)
- ✅ SUB with DOUBLE parameters (`value#`)
- ✅ SUB with STRING parameters (`msg$`)
- ✅ SUB with mixed parameter types
- ✅ Nested SUB calls
- ✅ SUB calling other SUBs

### Remaining Failures (8/123)
1. `test_mixed_types` - Type conversion edge case
2. `test_on_gosub` - Computed GOSUB dispatch
3. `test_on_goto` - Computed GOTO dispatch
4. `test_edge_cases` - Various edge cases
5. `gosub_if_control_flow` - GOSUB in IF blocks (compile error)
6. `mersenne_factors` - Complex Rosetta Code example
7. `mersenne_factors2` - Complex Rosetta Code example
8. `test_primes_sieve_working` - Optimized sieve implementation

---

## Design Lessons

### 1. **Consistency is Critical**
DEF FN and SUB should handle parameters identically. The inconsistency caused:
- Hard-to-debug errors (IL errors instead of semantic errors)
- Confusion between "function metadata" vs "function-local variables"

### 2. **Two-Phase Symbol Table**
Parameters need to exist in TWO places:
1. `FunctionSymbol.parameters` - For function signature/calling convention
2. `m_symbolTable.variables` - For codegen variable lookups

Both are necessary. Don't assume one implies the other.

### 3. **Test Coverage Gaps**
SUB parameters weren't properly tested until now. The issue existed since SUB was implemented but wasn't caught because:
- Most tests used DEF FN (which worked)
- Early SUB tests had no parameters or only simple cases

### 4. **Error Messages Matter**
The IL contained `# ERROR: variable not found: label_STRING` which made debugging straightforward. Good diagnostic output saves hours.

---

## Files Modified

| File | Lines Changed | Purpose |
|------|---------------|---------|
| `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` | +8 | Add SUB parameter insertion |

---

## Commit Information

**Commit:** `d6abdee`  
**Date:** February 2, 2025  
**Message:** "Fix SUB parameter handling - add parameters to symbol table"  
**Pushed to:** `origin/main`

---

## Related Work

This fix complements the earlier RETURN statement fix from the same session:
- **Commit `5ff3563`:** Fixed duplicate RETURN jumps (77% → 92% tests)
- **Commit `d6abdee`:** Fixed SUB parameters (92% → 93.5% tests)

Both fixes address fundamental semantic analysis issues that were masked by partial implementations.

---

## Conclusion

A simple 8-line fix that adds SUB parameters to the symbol table, matching the existing DEF FN behavior. This demonstrates the importance of:
- Consistent patterns across similar language constructs
- Proper symbol table management for scoped entities
- Thorough testing of language features with different parameter types

**Key Insight:** Just because parameters are stored in function metadata doesn't mean they're accessible as variables. Semantic analysis must explicitly register them in the symbol table with proper scoping.