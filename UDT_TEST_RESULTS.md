# User-Defined Types (UDT) Test Results

## Summary
Created focused tests for User-Defined Types (TYPE...END TYPE) to verify struct/record functionality.

**Date:** January 2025  
**Status:** 3 of 5 tests passing

---

## ✅ Working Features

### 1. Simple UDT with Single Field
- **Test:** `tests/types/test_udt_simple.bas`
- **Status:** PASSING ✅
- **Features:**
  - Define TYPE with one INTEGER field
  - Create variable of UDT type
  - Assign to field
  - Read from field
- **Example:**
  ```basic
  TYPE Point
    X AS INTEGER
  END TYPE
  DIM P AS Point
  P.X = 42
  ```

### 2. UDT with Multiple Fields (Mixed Types)
- **Test:** `tests/types/test_udt_twofields.bas`
- **Status:** PASSING ✅
- **Features:**
  - Multiple fields in one TYPE
  - Mixed INTEGER and DOUBLE fields
  - Independent field access
- **Example:**
  ```basic
  TYPE Point
    X AS INTEGER
    Y AS DOUBLE
  END TYPE
  P.X = 10
  P.Y = 20.5
  ```

### 3. Nested UDTs
- **Test:** `tests/types/test_udt_nested.bas`
- **Status:** PASSING ✅
- **Features:**
  - UDT field containing another UDT
  - Chained member access (e.g., `O.Item.Value`)
  - Proper offset calculation for nested structures
- **Example:**
  ```basic
  TYPE Inner
    Value AS INTEGER
  END TYPE
  TYPE Outer
    Item AS Inner
  END TYPE
  O.Item.Value = 99
  ```

---

## ❌ Known Issues

### 4. String Fields in UDTs
- **Test:** `tests/types/test_udt_string.bas`
- **Status:** FAILING ❌
- **Error:** "Type mismatch in assignment: cannot assign STRING to USER_DEFINED"
- **Problem:**
  - Semantic analyzer rejects string assignments to UDT fields
  - String fields are declared correctly in TYPE definition
  - Assignment validation incorrectly treats field as USER_DEFINED instead of STRING
- **Root Cause:** Member access type inference not properly returning field type
- **Impact:** Cannot use STRING fields in UDTs

### 5. Arrays of UDTs
- **Test:** `tests/types/test_udt_array.bas`
- **Status:** FAILING ❌
- **Error:** "Array subscript out of bounds: index 0 not in [171798691870, 0]"
- **Problem:**
  - Array descriptor bounds are corrupted for UDT element types
  - Lower bound shows as very large garbage value
  - Array allocation appears to succeed but bounds are wrong
- **Root Cause:** Array descriptor initialization doesn't properly handle UDT element sizes/types
- **Impact:** Cannot create arrays of user-defined types

---

## Test Files Created

1. `tests/types/test_udt_simple.bas` - Single INTEGER field (9 lines) ✅
2. `tests/types/test_udt_twofields.bas` - INTEGER and DOUBLE fields (11 lines) ✅
3. `tests/types/test_udt_string.bas` - STRING and INTEGER fields (11 lines) ❌
4. `tests/types/test_udt_nested.bas` - Nested UDT structures (12 lines) ✅
5. `tests/types/test_udt_array.bas` - Array of UDT elements (14 lines) ❌

---

## Parser Notes

**Keyword Conflict:** The field name `Data` conflicts with the `DATA` statement keyword.
- Attempted to use `Data AS Inner` in UDT definition
- Parser error: "Expected value in DATA statement"
- **Workaround:** Use different field names (e.g., `Item`, `Info`, `Content`)
- **Recommendation:** Consider allowing keywords as field names in TYPE context

---

## Architecture Observations

### What Works Well:
1. **Type definition parsing** - TYPE...END TYPE blocks parse correctly
2. **Symbol table** - UDT types are registered and looked up properly
3. **Field offset calculation** - Nested structures have correct memory layout
4. **Member access codegen** - Chained access (`.`) generates correct pointer arithmetic
5. **Scalar field types** - INTEGER and DOUBLE fields work perfectly

### Problem Areas:
1. **String field assignment** - Type checking too strict for STRING fields
2. **Array descriptors for UDTs** - Bounds initialization broken for non-scalar element types
3. **Keyword restrictions** - Cannot use common words as field names

---

## Recommended Fixes

### High Priority:

1. **Fix String Fields**
   - Location: `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`
   - Issue: `validateLetStatement()` or member access type inference
   - Solution: Ensure `inferMemberAccessType()` returns actual field type, not container type
   - Estimated effort: Small fix, likely 1-2 lines

2. **Fix UDT Arrays**
   - Location: Array descriptor initialization (possibly in `emitDim()` or array runtime)
   - Issue: Bounds not set correctly when element type is USER_DEFINED
   - Solution: Check array descriptor setup for UDT element types, ensure size calculation is correct
   - Estimated effort: Medium, requires understanding array descriptor layout

### Medium Priority:

3. **Allow Keywords as Field Names**
   - Location: Parser TYPE context handling
   - Issue: Parser doesn't distinguish statement context from field definition context
   - Solution: Add special handling in TYPE block to allow keywords as identifiers
   - Estimated effort: Medium, parser modification

---

## Testing Approach

Followed **minimal, focused test strategy**:
- Each test file tests ONE UDT feature
- Simple, clear test cases (9-14 lines)
- Immediate PASS/FAIL feedback
- Easy to isolate and debug issues

This approach successfully identified 2 specific bugs:
1. String field type mismatch
2. UDT array bounds corruption

---

## Next Steps

1. **Immediate:** Fix string field assignment validation
2. **Short-term:** Fix UDT array descriptor initialization
3. **Medium-term:** Add more UDT tests:
   - Arrays of nested UDTs
   - Passing UDTs to/from SUB/FUNCTION
   - REDIM with UDT arrays
   - UDT fields with all numeric types (BYTE, SHORT, LONG)
4. **Long-term:** Allow keywords as field names in TYPE definitions

---

## Success Rate

**Passing:** 3 of 5 tests (60%)

UDT core functionality is solid. The two failing tests represent important use cases that need fixes:
- STRING fields are common in real-world structures
- Arrays of structs are fundamental for many applications

Once these issues are resolved, UDT support will be highly functional.