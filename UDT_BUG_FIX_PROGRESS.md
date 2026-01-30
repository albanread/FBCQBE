# UDT Bug Fix Progress Report

**Date:** January 2025  
**Session:** Bug Fix Implementation  
**Status:** 🟡 Bug #1 Partially Fixed, Bug #2 Not Started

---

## Overview

Implementing fixes for the two critical UDT bugs identified in the implementation review:

1. **Bug #1:** String field type inference (HIGH PRIORITY) - 🟡 75% Complete
2. **Bug #2:** Arrays of UDTs descriptor initialization (HIGH PRIORITY) - ⏳ Not Started

---

## Bug #1: String Field Type Inference

### Problem Statement

**Original Issue:**
```basic
TYPE Person
    Name AS STRING
    Age AS INTEGER
END TYPE
DIM P AS Person
P.Name = "Alice"    ' ERROR: Type mismatch in assignment
```

**Error Message:**
```
Type mismatch in assignment: cannot assign STRING to USER_DEFINED
```

**Root Cause:** Multiple issues in the semantic analysis and code generation pipeline.

---

### Fixes Implemented

#### 1. ✅ Fixed `inferMemberAccessType()` - Semantic Analysis

**Location:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp:2416`

**Problem:** Function always returned `VariableType::UNKNOWN` instead of looking up actual field type.

**Solution:**
```cpp
VariableType SemanticAnalyzer::inferMemberAccessType(const MemberAccessExpression& expr) {
    // Get base object type name
    std::string baseTypeName;
    
    if (expr.object->getType() == ASTNodeType::EXPR_VARIABLE) {
        VariableSymbol* varSym = lookupVariable(varExpr->name);
        if (varSym && varSym->type == VariableType::USER_DEFINED) {
            baseTypeName = varSym->typeName;
        }
    }
    // ... handle array access and nested members ...
    
    // Look up type definition
    TypeSymbol* typeSymbol = lookupType(baseTypeName);
    const TypeSymbol::Field* field = typeSymbol->findField(expr.memberName);
    
    // Return actual field type!
    return field->isBuiltIn ? field->builtInType : VariableType::USER_DEFINED;
}
```

**Result:** Type inference now returns correct field types (STRING, INTEGER, etc.)

---

#### 2. ✅ Fixed `validateLetStatement()` - Assignment Validation

**Location:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp:1576`

**Problem:** Assignment validation didn't handle `memberChain` for UDT field assignment.

**Solution:** Added full member chain traversal:
```cpp
void SemanticAnalyzer::validateLetStatement(const LetStatement& stmt) {
    // ... existing code ...
    
    VariableType targetType;
    
    // Handle member access (UDT field assignment)
    if (!stmt.memberChain.empty()) {
        // Get base variable type
        VariableSymbol* varSym = lookupVariable(stmt.variable);
        TypeSymbol* typeSymbol = lookupType(varSym->typeName);
        
        // Navigate through member chain
        TypeSymbol* currentType = typeSymbol;
        for (size_t i = 0; i < stmt.memberChain.size(); ++i) {
            const TypeSymbol::Field* field = currentType->findField(memberName);
            
            // Last member - get its type
            if (i == stmt.memberChain.size() - 1) {
                targetType = field->isBuiltIn ? field->builtInType : VariableType::USER_DEFINED;
            } else {
                // Nested member - must be UDT
                currentType = lookupType(field->typeName);
            }
        }
    }
    // ... existing array/variable handling ...
    
    checkTypeCompatibility(targetType, valueType, stmt.location, "assignment");
}
```

**Result:** Assignment validation now correctly validates field type vs expression type.

---

#### 3. ✅ Fixed `emitLet()` - Store Operations

**Location:** `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp:473`

**Problem:** Used legacy `VariableType` which maps INTEGER to 'l' (64-bit), but UDT INTEGER fields should be 'w' (32-bit).

**Solution:** Use TypeDescriptor for correct QBE type mapping:
```cpp
// Get field type from TypeDescriptor (correct for INTEGER = 'w' not 'l')
TypeDescriptor fieldTypeDesc = finalField->typeDesc;
std::string qbeType = fieldTypeDesc.toQBEType();  // Returns "w" for INTEGER

// Type conversion if needed
if (fieldTypeLegacy != exprType) {
    std::string convertedTemp = promoteToType(valueTemp, exprType, fieldTypeLegacy);
    
    // If promoteToType returned 'l' but field needs 'w', truncate
    if (qbeType == "w" && fieldTypeLegacy == VariableType::INT) {
        std::string truncTemp = allocTemp("w");
        emit("    " + truncTemp + " =w copy " + convertedTemp + "\n");
        valueTemp = truncTemp;
    }
}

// Store with correct instruction
if (qbeType == "w") {
    emit("    storew " + valueTemp + ", " + currentPtr + "\n");
} else if (qbeType == "d") {
    emit("    stored " + valueTemp + ", " + currentPtr + "\n");
} else if (qbeType == "l") {
    emit("    storel " + valueTemp + ", " + currentPtr + "\n");
}
```

**Result:** Store operations now use correct QBE types (`storew` for INTEGER fields).

---

### ❌ Remaining Issue: Load Operations

**Problem:** Member access read still uses wrong load instruction.

**Current QBE Output:**
```qbe
# P.X = 42 (store works correctly)
%t2 =l copy %var_P
storew 42, %t2           # ✅ CORRECT: storew for INTEGER field

# IF P.X = 42 (load is wrong)
%t9 =l copy %var_P
%t10 =l loadl %t9        # ❌ WRONG: should be loadsw
%t11 =d copy d_42.000000
%t12 =d sltof %t10       # ❌ Converting garbage (read 64 bits, only stored 32)
```

**Location to Fix:** `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp:1899` - `emitMemberAccessExpr()`

**Required Change:**
```cpp
std::string QBECodeGenerator::emitMemberAccessExpr(const MemberAccessExpression* expr) {
    // ... existing offset calculation ...
    
    // 6. Load value based on field type
    if (field->isBuiltIn) {
        // Use TypeDescriptor for correct load operation
        TypeDescriptor fieldTypeDesc = field->typeDesc;
        std::string qbeType = fieldTypeDesc.toQBEType();
        std::string resultTemp = allocTemp(qbeType);
        
        if (qbeType == "w") {
            // INTEGER field: sign-extend 32-bit word to 64-bit for arithmetic
            std::string widened = allocTemp("l");
            emit("    " + resultTemp + " =w loadw " + memberPtr + "\n");
            emit("    " + widened + " =l extsw " + resultTemp + "\n");
            return widened;  // Return as 'l' for compatibility
        } else if (qbeType == "d") {
            emit("    " + resultTemp + " =d loadd " + memberPtr + "\n");
        } else if (qbeType == "l") {
            emit("    " + resultTemp + " =l loadl " + memberPtr + "\n");
        }
        return resultTemp;
    }
    // ... nested UDT handling ...
}
```

---

### Test Results

**Test:** `tests/types/test_udt_simple.bas`
```basic
TYPE Point
    X AS INTEGER
END TYPE
DIM P AS Point
P.X = 42
PRINT "P.X = "; P.X
IF P.X = 42 THEN PRINT "PASS" ELSE PRINT "FAIL"
```

**Current Output:**
```
P.X = 42
FAIL           ← Wrong! Comparison fails due to load bug
```

**Expected Output:**
```
P.X = 42
PASS
```

---

**Test:** `tests/types/test_udt_string.bas`
```basic
TYPE Person
    Name AS STRING
    Age AS INTEGER
END TYPE
DIM P AS Person
P.Name = "Alice"
P.Age = 25
```

**Status:**
- ✅ Compilation succeeds (no semantic errors)
- ✅ String field assignment works
- ❌ Integer field read fails (load bug)

---

### Summary: Bug #1 Status

| Component | Status | Notes |
|-----------|--------|-------|
| Semantic type inference | ✅ Fixed | `inferMemberAccessType()` works |
| Assignment validation | ✅ Fixed | `validateLetStatement()` handles member chain |
| Store operations | ✅ Fixed | Uses `storew` for INTEGER fields |
| Load operations | ❌ Broken | Uses `loadl` instead of `loadsw` |
| String fields | ✅ Working | Assignment and reading work |
| Numeric fields | 🟡 Partial | Store works, load broken |

**Overall Progress:** 75% complete

**Estimated Time to Complete:** 30-60 minutes (fix load operations)

---

## Bug #2: Arrays of UDTs

### Problem Statement

**Original Issue:**
```basic
TYPE Point
    X AS INTEGER
    Y AS INTEGER
END TYPE
DIM Points(5) AS Point
Points(0).X = 100    ' ERROR: Array subscript out of bounds
```

**Error Message:**
```
Array subscript out of bounds: index 0 not in [171798691870, 0]
```

**Root Cause:** ArrayDescriptor bounds corrupted for UDT element types.

---

### Analysis

**ArrayDescriptor Layout:**
```c
typedef struct {
    void* data;         // offset 0
    int lowerBound1;    // offset 8   ← CORRUPTED (huge garbage value)
    int upperBound1;    // offset 16  ← CORRUPTED
    int lowerBound2;    // offset 24
    int upperBound2;    // offset 32
    int elementSize;    // offset 40  ← May be wrong for UDTs
    int dimensions;     // offset 48
    int typeSuffix;     // offset 56
} ArrayDescriptor;
```

**Likely Causes:**
1. Element size calculation doesn't handle USER_DEFINED types
2. Descriptor initialization skips or overwrites bounds for UDT arrays
3. Type size calculation may not account for padding/alignment

---

### Required Fixes

#### 1. Add UDT Element Size Calculation

**Location:** Array allocation code (need to identify exact location)

**Current:**
```cpp
size_t elementSize = getBasicTypeSize(arraySym.type);  // Fails for USER_DEFINED
```

**Needed:**
```cpp
size_t getElementSize(const ArraySymbol& arraySym) {
    if (arraySym.type == VariableType::USER_DEFINED) {
        // Look up UDT and calculate size
        const TypeSymbol* typeSymbol = lookupType(arraySym.asTypeName);
        return typeSymbol ? calculateTypeSize(typeName) : 0;
    }
    return getBasicTypeSize(arraySym.type);
}
```

#### 2. Fix Descriptor Bounds Initialization

**Location:** `emitDim()` for UDT arrays

**Current:** Possibly skipping bounds initialization for UDT arrays

**Needed:**
```qbe
# After allocating data pointer
%desc_lb =l add %desc, 8
storew <lowerBound>, %desc_lb

%desc_ub =l add %desc, 16
storew <upperBound>, %desc_ub

%desc_size =l add %desc, 40
storew <elementSize>, %desc_size
```

#### 3. Implement `calculateTypeSize()`

**Location:** `qbe_codegen_helpers.cpp`

**Needed:**
```cpp
size_t QBECodeGenerator::calculateTypeSize(const std::string& typeName) {
    const TypeSymbol* typeSym = getTypeSymbol(typeName);
    if (!typeSym) return 0;
    
    size_t totalSize = 0;
    for (const auto& field : typeSym->fields) {
        size_t fieldSize = getFieldSize(field);
        size_t alignment = getAlignment(field);
        
        // Align offset
        if (totalSize % alignment != 0) {
            totalSize += alignment - (totalSize % alignment);
        }
        
        totalSize += fieldSize;
    }
    
    // Align total struct size to largest field alignment
    size_t maxAlign = getMaxAlignment(typeSym);
    if (totalSize % maxAlign != 0) {
        totalSize += maxAlign - (totalSize % maxAlign);
    }
    
    return totalSize;
}
```

---

### Status

**Bug #2:** ⏳ Not Started

**Estimated Time:** 2-3 hours
- Locate array allocation code
- Add UDT size calculation
- Fix descriptor initialization
- Test with various UDT arrays

---

## Next Steps

### Immediate Priority

1. **Fix Bug #1 Load Operations** (30-60 minutes)
   - Update `emitMemberAccessExpr()` to use correct load instruction
   - Use `loadsw` for INTEGER fields
   - Test all UDT tests to verify fix

2. **Verify Bug #1 Complete** (15 minutes)
   - Run `tests/types/test_udt_simple.bas` → should PASS
   - Run `tests/types/test_udt_string.bas` → should PASS
   - Run `tests/types/test_udt_twofields.bas` → should PASS
   - Run `tests/types/test_udt_nested.bas` → should PASS

3. **Start Bug #2** (2-3 hours)
   - Locate array descriptor initialization code
   - Implement UDT element size calculation
   - Fix bounds initialization
   - Test `tests/types/test_udt_array.bas` → should PASS

### Testing Strategy

**After Each Fix:**
```bash
# Rebuild
cd qbe_basic_integrated && ./build_qbe_basic.sh

# Test single file
./qbe_basic_integrated/qbe_basic tests/types/test_udt_simple.bas > test.s
cc test.s qbe_basic_integrated/runtime/.obj/*.o -o test
./test

# Run all UDT tests
for test in tests/types/test_udt_*.bas; do
    echo "=== $test ==="
    ./qbe_basic_integrated/qbe_basic "$test" > test.s 2>&1
    if cc test.s qbe_basic_integrated/runtime/.obj/*.o -o test 2>/dev/null; then
        ./test
    else
        echo "Compilation failed"
    fi
    echo
done
```

---

## Files Modified

### Bug #1 Fixes

1. `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`
   - Lines 2416-2484: `inferMemberAccessType()` - Complete rewrite
   - Lines 1576-1645: `validateLetStatement()` - Added member chain handling

2. `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp`
   - Lines 473-512: `emitLet()` - Use TypeDescriptor for field types

### Bug #1 Remaining

3. `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp`
   - Lines 1899-1980: `emitMemberAccessExpr()` - Need to fix load operations

### Bug #2 (Not Started)

4. Array allocation code (location TBD)
5. `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp` - Add `calculateTypeSize()`
6. Descriptor initialization code (location TBD)

---

## Confidence Assessment

**Bug #1 Completion:** 🟢 High confidence
- Root causes identified
- Most fixes implemented
- One remaining fix is straightforward
- Clear test cases available

**Bug #2 Completion:** 🟡 Medium confidence
- Root cause understood
- Solution approach clear
- Need to locate exact code
- May have additional edge cases

**Timeline:** 3-5 hours total to complete both bugs

---

## Success Criteria

### Bug #1 Success
- [ ] All semantic errors resolved
- [ ] Store operations use correct QBE types
- [ ] Load operations use correct QBE types
- [ ] `test_udt_simple.bas` prints "PASS"
- [ ] `test_udt_string.bas` prints "PASS"
- [ ] `test_udt_twofields.bas` prints "PASS"
- [ ] `test_udt_nested.bas` prints "PASS"

### Bug #2 Success
- [ ] Array descriptor bounds correctly initialized
- [ ] Element size calculated for UDTs
- [ ] `test_udt_array.bas` prints "PASS"
- [ ] Multi-dimensional UDT arrays work
- [ ] REDIM with UDT arrays works

### Overall Success
- [ ] 5 of 5 UDT tests passing (100%)
- [ ] No regression in other tests
- [ ] Documentation updated
- [ ] Commit and push changes

---

**End of Progress Report**