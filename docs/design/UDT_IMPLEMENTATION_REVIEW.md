# User-Defined Types (UDT) Implementation Review

**Last Updated:** January 2025  
**Status:** ✅ Core implementation complete, 🔴 2 critical bugs blocking full functionality

---

## Executive Summary

FasterBASIC has **substantial UDT support already implemented**, covering the full pipeline from parsing through code generation. The architecture is sound and follows modern compiler design patterns.

**What Works (60%):**
- ✅ TYPE...END TYPE declaration parsing
- ✅ Type registration and validation  
- ✅ Scalar UDT variables (`DIM P AS Point`)
- ✅ Member access for numeric fields (`P.X`, `P.Y`)
- ✅ Nested UDTs (`O.Item.Value`)
- ✅ QBE code generation with correct offsets

**What's Broken (40%):**
- ❌ String fields in UDTs (type inference bug)
- ❌ Arrays of UDTs (descriptor initialization bug)

**Estimate to Fix:** 2-4 hours for both issues

---

## 1. Parser Implementation ✅

### Location
- `fsh/FasterBASICT/src/fasterbasic_parser.cpp` - `parseTypeDeclarationStatement()` (lines 2829-2920)
- `fsh/FasterBASICT/src/fasterbasic_ast.h` - `TypeDeclarationStatement` class

### Features Implemented

```basic
TYPE Point
    X AS INTEGER
    Y AS DOUBLE
END TYPE

TYPE Nested
    Item AS Point       ' Nested UDT support
    Name AS STRING      ' String fields (parsed correctly)
END TYPE

' Both END TYPE and ENDTYPE are supported
```

### Parser Capabilities

1. **Syntax Recognition**
   - `TYPE TypeName` header
   - Field declarations: `FieldName AS TypeName`
   - Both built-in types (INTEGER, DOUBLE, STRING) and user-defined types
   - `END TYPE` or `ENDTYPE` terminator

2. **AST Node Structure**
   ```cpp
   class TypeDeclarationStatement {
       std::string typeName;
       std::vector<Field> fields;
       
       struct Field {
           std::string name;
           std::string typeName;
           TokenType builtInType;
           bool isBuiltIn;
       };
   };
   ```

3. **Type Keyword Detection**
   - `isTypeKeyword()` distinguishes built-in vs user-defined types
   - Proper handling of AS clause

### Known Parser Limitations

- **Keyword field names blocked**: Cannot use `Data`, `Type`, etc. as field names (parser conflict)
- **No array fields**: `Field(10) AS INTEGER` not supported in TYPE blocks
- **No default values**: Cannot initialize fields in TYPE declaration

---

## 2. Semantic Analysis ✅

### Location
- `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - `processTypeDeclarationStatement()` (lines 449-560)
- Symbol table: `std::unordered_map<std::string, TypeSymbol> types`

### Features Implemented

1. **Type Registration**
   ```cpp
   void SemanticAnalyzer::processTypeDeclarationStatement(const TypeDeclarationStatement* stmt) {
       // Allocates unique type ID
       int udtTypeId = m_symbolTable.allocateTypeId(stmt->typeName);
       
       // Creates TypeSymbol with fields
       TypeSymbol typeSymbol(stmt->typeName);
       
       // Validates duplicate field names
       std::unordered_set<std::string> fieldNames;
       
       // Stores in symbol table
       m_symbolTable.types[stmt->typeName] = typeSymbol;
   }
   ```

2. **Validation Checks**
   - ✅ Duplicate type name detection
   - ✅ Duplicate field name detection within a type
   - ✅ Built-in type validation
   - ✅ Nested UDT reference tracking (resolved in second pass)

3. **SIMD Type Detection** (Advanced Feature)
   - Automatically detects **PAIR** pattern: 2 consecutive DOUBLEs
   - Automatically detects **QUAD** pattern: 4 consecutive SINGLEs (floats)
   - Enables potential vectorization optimizations

4. **TypeDescriptor Integration**
   ```cpp
   TypeDescriptor fieldTypeDesc;
   
   // Built-in type
   fieldTypeDesc = TypeDescriptor(BaseType::INTEGER);
   
   // User-defined type
   fieldTypeDesc = TypeDescriptor(BaseType::USER_DEFINED);
   fieldTypeDesc.udtName = field.typeName;
   ```

### TypeSymbol Structure

```cpp
struct TypeSymbol {
    std::string name;
    std::vector<Field> fields;
    SourceLocation declaration;
    SIMDType simdType;  // NONE, PAIR, QUAD
    
    struct Field {
        std::string name;
        TypeDescriptor typeDesc;  // Full type information
        bool isBuiltIn;
        TokenType builtInType;    // For legacy compatibility
    };
    
    const Field* findField(const std::string& fieldName) const;
    size_t getSize() const;  // Total struct size in bytes
};
```

---

## 3. Variable Declaration ✅

### Location
- `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - `processDimStatement()` (lines 843-1070)

### DIM AS TypeName Support

```basic
DIM Player AS Point          ' Scalar UDT variable
DIM Enemies(10) AS Sprite    ' Array of UDT (BROKEN - see bug #2)
```

### Implementation

```cpp
void SemanticAnalyzer::processDimStatement(const DimStatement& stmt) {
    if (arrayDim.dimensions.empty() && arrayDim.hasAsType) {
        // Scalar UDT variable
        
        // Check type exists
        if (m_symbolTable.types.find(arrayDim.asTypeName) == m_symbolTable.types.end()) {
            error(SemanticErrorType::UNDEFINED_TYPE, ...);
            return;
        }
        
        // Create TypeDescriptor
        TypeDescriptor typeDesc(BaseType::USER_DEFINED);
        typeDesc.udtName = arrayDim.asTypeName;
        typeDesc.udtTypeId = m_symbolTable.allocateTypeId(arrayDim.asTypeName);
        
        // Declare variable
        VariableSymbol* sym = declareVariableD(arrayDim.name, typeDesc, ...);
        sym->functionScope = m_currentFunctionName;
    }
}
```

### Variable Symbol Storage

```cpp
struct VariableSymbol {
    std::string name;
    TypeDescriptor typeDesc;     // New: Complete type info
    VariableType type;           // Legacy: for compatibility
    std::string typeName;        // Legacy: UDT name
    bool isDeclared;
    std::string functionScope;
    
    // Constructor automatically populates legacy fields
    VariableSymbol(const std::string& n, const TypeDescriptor& td) {
        typeDesc = td;
        type = descriptorToLegacyType(td);
        if (td.isUserDefined()) {
            typeName = td.udtName;  // ← Critical: stores UDT name
        }
    }
};
```

**Key Point:** UDT variables store their type name in **two places**:
1. `typeDesc.udtName` (new system)
2. `typeName` (legacy field for codegen compatibility)

---

## 4. Member Access - Parsing & AST ✅

### Location
- `fsh/FasterBASICT/src/fasterbasic_ast.h` - `MemberAccessExpression` class (lines 418-434)

### AST Node Structure

```cpp
class MemberAccessExpression : public Expression {
public:
    ExpressionPtr object;       // Base object (can be nested)
    std::string memberName;     // Field name
    
    // Examples:
    // P.X          → object=VariableExpression("P"), memberName="X"
    // O.Item.Value → object=MemberAccessExpression(...), memberName="Value"
};
```

### Supported Patterns

1. **Simple member access**: `P.X`
2. **Chained member access**: `O.Item.Value`
3. **Array element member access**: `Enemies(i).Health`
4. **Nested expressions**: `(GetPlayer()).Position.X`

---

## 5. Member Access - Code Generation ✅ (Mostly)

### Location
- `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp` - `emitMemberAccessExpr()` (lines 1899-1980)
- `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp` - Field offset calculation

### Code Generation Flow

```cpp
std::string QBECodeGenerator::emitMemberAccessExpr(const MemberAccessExpression* expr) {
    // 1. Evaluate base object (returns pointer to struct)
    std::string baseTemp = emitExpression(expr->object.get());
    
    // 2. Determine type of base object
    std::string baseTypeName = inferMemberAccessType(expr->object.get());
    
    // 3. Look up type definition
    const TypeSymbol* typeSymbol = getTypeSymbol(baseTypeName);
    
    // 4. Find field in type
    const TypeSymbol::Field* field = typeSymbol->findField(expr->memberName);
    
    // 5. Calculate field offset
    size_t offset = calculateFieldOffset(baseTypeName, expr->memberName);
    
    // 6. Generate pointer arithmetic
    std::string memberPtr = allocTemp("l");
    emit("    " + memberPtr + " =l add " + baseTemp + ", " + std::to_string(offset) + "\n");
    
    // 7. Load value (or return pointer for nested UDTs)
    if (field->isBuiltIn) {
        std::string qbeType = getQBEType(field->builtInType);
        std::string result = allocTemp(qbeType);
        emit("    " + result + " =" + qbeType + " load" + qbeType + " " + memberPtr + "\n");
        return result;
    } else {
        // Nested UDT - return pointer for further access
        return memberPtr;
    }
}
```

### Generated QBE Example

```basic
TYPE Point
    X AS INTEGER
    Y AS DOUBLE
END TYPE
DIM P AS Point
P.X = 42
Z = P.X
```

Generated QBE:
```qbe
# DIM P AS Point
%P =l alloc8 16              # Allocate 16 bytes (4 int + 8 double + padding)

# P.X = 42
%t1 =l copy %P               # Get base pointer
%t2 =l add %t1, 0            # Field X at offset 0
storew 42, %t2               # Store integer

# Z = P.X
%t3 =l copy %P               # Get base pointer
%t4 =l add %t3, 0            # Field X at offset 0
%t5 =w loadw %t4             # Load integer
```

### Field Offset Calculation

```cpp
size_t QBECodeGenerator::calculateFieldOffset(const std::string& typeName, 
                                                const std::string& fieldName) {
    const TypeSymbol* typeSymbol = getTypeSymbol(typeName);
    size_t offset = 0;
    
    for (const auto& field : typeSymbol->fields) {
        if (field.name == fieldName) {
            return offset;
        }
        
        // Add field size with proper alignment
        size_t fieldSize = getFieldSize(field);
        size_t alignment = getAlignment(field);
        offset = alignTo(offset, alignment);
        offset += fieldSize;
    }
    
    return offset;
}
```

**Alignment Rules** (matches C struct layout):
- INTEGER (4 bytes) → 4-byte aligned
- DOUBLE (8 bytes) → 8-byte aligned
- STRING (8 bytes pointer) → 8-byte aligned
- Nested UDT → alignment of largest field

---

## 6. Test Coverage

### Location
- `tests/types/test_udt_*.bas`

### Test Results

| Test | Status | Description |
|------|--------|-------------|
| `test_udt_simple.bas` | ✅ PASS | Single INTEGER field |
| `test_udt_twofields.bas` | ✅ PASS | INTEGER + DOUBLE fields |
| `test_udt_nested.bas` | ✅ PASS | Nested UDT (chained access) |
| `test_udt_string.bas` | ❌ FAIL | STRING field (Bug #1) |
| `test_udt_array.bas` | ❌ FAIL | Array of UDT (Bug #2) |

**Success Rate:** 3 of 5 tests passing (60%)

---

## 7. Critical Bugs 🔴

### Bug #1: String Fields Type Inference Broken

**Test:** `tests/types/test_udt_string.bas`

```basic
TYPE Person
    Name AS STRING
    Age AS INTEGER
END TYPE
DIM P AS Person
P.Name = "Alice"    ' ← ERROR: Type mismatch
```

**Error Message:**
```
Type mismatch in assignment: cannot assign STRING to USER_DEFINED
```

**Root Cause:**

The member access type inference returns the **container type** (USER_DEFINED) instead of the **field type** (STRING).

Location: `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - `inferMemberAccessType()` (line 2415)

```cpp
VariableType SemanticAnalyzer::inferMemberAccessType(const MemberAccessExpression& expr) {
    // ... determine base type ...
    
    // BUG: Returns UNKNOWN instead of looking up field type
    return VariableType::UNKNOWN;  // ← This is the problem
}
```

**The Fix (Estimated 1-2 hours):**

```cpp
VariableType SemanticAnalyzer::inferMemberAccessType(const MemberAccessExpression& expr) {
    // 1. Get base object type
    std::string baseTypeName = getBaseObjectTypeName(expr.object.get());
    
    // 2. Look up type definition
    const TypeSymbol* typeSymbol = lookupType(baseTypeName);
    if (!typeSymbol) return VariableType::UNKNOWN;
    
    // 3. Find field
    const TypeSymbol::Field* field = typeSymbol->findField(expr.memberName);
    if (!field) return VariableType::UNKNOWN;
    
    // 4. Return FIELD type, not container type
    if (field->isBuiltIn) {
        return tokenTypeToVariableType(field->builtInType);
    } else {
        return VariableType::USER_DEFINED;  // Nested UDT
    }
}
```

**Testing Plan:**
1. Fix `inferMemberAccessType()` to return actual field type
2. Verify `test_udt_string.bas` passes
3. Test nested UDTs still work
4. Test mixed type assignments

---

### Bug #2: Arrays of UDTs - Descriptor Initialization

**Test:** `tests/types/test_udt_array.bas`

```basic
TYPE Point
    X AS INTEGER
    Y AS INTEGER
END TYPE
DIM Points(5) AS Point
Points(0).X = 100    ' ← ERROR: Array subscript out of bounds
```

**Error Message:**
```
Array subscript out of bounds: index 0 not in [171798691870, 0]
```

**Root Cause:**

The ArrayDescriptor bounds are corrupted when element type is USER_DEFINED. The lower bound shows as a huge garbage value (171798691870 instead of 0 or 1).

**ArrayDescriptor Layout (Reminder):**
```c
typedef struct {
    void* data;         // offset 0
    int lowerBound1;    // offset 8   ← CORRUPTED
    int upperBound1;    // offset 16  ← CORRUPTED
    int lowerBound2;    // offset 24
    int upperBound2;    // offset 32
    int elementSize;    // offset 40  ← May be wrong for UDTs
    int dimensions;     // offset 48
    int typeSuffix;     // offset 56
} ArrayDescriptor;
```

**Likely Causes:**

1. **Element size calculation** doesn't handle USER_DEFINED types
   - Location: Array allocation code
   - Needs to call `calculateTypeSize(udtName)`

2. **Descriptor initialization** may not set bounds correctly for UDT arrays
   - Location: `emitDim()` or array runtime initialization
   - Bounds fields may be skipped or overwritten

**The Fix (Estimated 2-3 hours):**

Step 1: Add UDT element size calculation
```cpp
size_t getElementSize(const ArraySymbol& arraySym) {
    if (arraySym.type == VariableType::USER_DEFINED) {
        // Look up UDT and calculate size
        const TypeSymbol* typeSymbol = lookupType(arraySym.typeName);
        return typeSymbol ? typeSymbol->getSize() : 0;
    }
    // ... existing built-in type sizes ...
}
```

Step 2: Fix descriptor initialization in `emitDim()`
```cpp
// After allocating data pointer
emit("    %desc_bounds1 =l add %desc, 8\n");
emit("    storew " + std::to_string(lowerBound) + ", %desc_bounds1\n");
emit("    %desc_bounds2 =l add %desc, 16\n");
emit("    storew " + std::to_string(upperBound) + ", %desc_bounds2\n");
```

Step 3: Verify runtime doesn't overwrite bounds

**Testing Plan:**
1. Fix element size calculation for UDTs
2. Fix descriptor bounds initialization
3. Verify `test_udt_array.bas` passes
4. Test multi-dimensional UDT arrays
5. Test REDIM with UDT arrays

---

## 8. Missing Features (Not Yet Implemented)

### 8.1 Member Assignment Codegen

**Status:** Unknown (needs testing)

```basic
P.X = 42    ' Does this work on LEFT side of assignment?
```

Likely location: `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp` - `emitLet()`

The `emitLet()` function needs to handle `MemberAccessExpression` on the LHS:
```cpp
if (stmt.variable->getType() == ASTNodeType::EXPR_MEMBER_ACCESS) {
    // Get field address
    std::string fieldPtr = emitMemberAccessExpr(...);
    
    // Store value (don't load)
    std::string valueTemp = emitExpression(stmt.value);
    emit("    store" + qbeType + " " + valueTemp + ", " + fieldPtr + "\n");
}
```

**Testing:** Create `test_udt_assign.bas` to verify member assignment works.

---

### 8.2 Passing UDTs to Functions

```basic
SUB DrawPoint(P AS Point)
    PRINT P.X, P.Y
END SUB

DIM MyPoint AS Point
DrawPoint(MyPoint)
```

**Required Changes:**
1. Parser: Handle `AS TypeName` in parameter lists
2. Semantic: Type check arguments against UDT parameters
3. Codegen: Pass struct pointer (8 bytes) vs pass-by-value (copy struct)

**Design Decision:** Pass by reference (pointer) or by value (copy)?
- **Recommendation:** Pass by reference (matches BASIC semantics, more efficient)

---

### 8.3 Returning UDTs from Functions

```basic
FUNCTION GetOrigin() AS Point
    DIM P AS Point
    P.X = 0
    P.Y = 0
    RETURN P
END FUNCTION
```

**QBE Challenge:** QBE doesn't support struct return types directly.

**Solution Options:**
1. **Hidden pointer parameter** (C calling convention)
   ```qbe
   function $GetOrigin(l %ret_ptr) {
       # Initialize struct at ret_ptr
   }
   ```

2. **Return pointer to heap-allocated struct**
   ```qbe
   function l $GetOrigin() {
       %ptr =l call $malloc(l 16)
       # Initialize struct
       ret %ptr
   }
   ```

**Recommendation:** Option 1 (matches C ABI, no heap allocation)

---

### 8.4 Array Fields in UDTs

```basic
TYPE Grid
    Cells(10, 10) AS INTEGER    ' Fixed-size array field
END TYPE
```

**Complexity:** High - requires embedding array descriptors in structs

**Alternative:** Use dynamic arrays at module level instead

---

### 8.5 Keyword Field Names

```basic
TYPE DataRecord
    Data AS STRING    ' ERROR: Parser conflict with DATA statement
END TYPE
```

**Fix:** Context-aware keyword recognition in TYPE blocks

**Workaround:** Use different field names (e.g., `Value`, `Content`, `Info`)

---

## 9. QBE Integration Details

### How QBE Helps with UDTs

QBE doesn't have native struct/aggregate types, but provides the primitives needed:

1. **Memory allocation**
   ```qbe
   %ptr =l alloc8 16    # 16-byte aligned allocation
   ```

2. **Pointer arithmetic**
   ```qbe
   %field =l add %base, 8    # offset to field at +8 bytes
   ```

3. **Typed loads/stores**
   ```qbe
   %val =w loadw %field      # load 4-byte int
   %val =d loadd %field      # load 8-byte double
   ```

4. **Type safety**
   - QBE enforces operand types on every instruction
   - Type mismatches caught at QBE IL validation stage

### Generated Code Quality

**Efficient:**
- Direct memory access (no indirection)
- Compile-time offset calculation (no runtime overhead)
- Proper alignment for SIMD potential

**Example Benchmark:**
```basic
FOR i = 1 TO 1000000
    P.X = i
    Sum = Sum + P.X
NEXT
```

QBE generates tight loop with no function calls - comparable to hand-written C.

---

## 10. Comparison with Other BASIC Compilers

### FreeBASIC
- ✅ Full UDT support with methods
- ✅ Inheritance and polymorphism
- ✅ Operator overloading
- ❌ More complex (C++-like)

**FasterBASIC advantage:** Simpler model, easier to understand and debug

### QB64
- ✅ QB-compatible UDT syntax
- ✅ Arrays of UDTs
- ❌ No nested UDTs
- ❌ Limited type checking

**FasterBASIC advantage:** Nested UDTs, better type safety

### Visual Basic 6
- ✅ Full UDT support
- ✅ Arrays of UDTs
- ❌ No nested UDTs
- ❌ COM overhead

**FasterBASIC advantage:** Native code, no runtime overhead

---

## 11. Architecture Strengths

### What's Done Right

1. **Separation of Concerns**
   - Parser handles syntax
   - Semantic analyzer validates semantics
   - Codegen produces QBE IL
   - Clean interfaces between phases

2. **TypeDescriptor System**
   - Modern type representation
   - Extensible for future types
   - Proper legacy compatibility

3. **Offset Calculation**
   - Centralized field offset logic
   - Alignment-aware
   - Cached for performance

4. **Nested UDT Support**
   - Recursive type resolution
   - Proper pointer return for chaining
   - Clean AST representation

5. **Test-Driven Validation**
   - Small, focused tests
   - Clear pass/fail criteria
   - Easy to debug failures

---

## 12. Recommended Next Steps

### Immediate (Fix Critical Bugs)

**Priority 1: Fix String Field Bug (1-2 hours)**
- [ ] Update `inferMemberAccessType()` to return actual field type
- [ ] Test with `test_udt_string.bas`
- [ ] Verify nested UDTs still work

**Priority 2: Fix UDT Array Bug (2-3 hours)**
- [ ] Add `getElementSize()` for USER_DEFINED types
- [ ] Fix ArrayDescriptor initialization in `emitDim()`
- [ ] Test with `test_udt_array.bas`
- [ ] Test multi-dimensional UDT arrays

### Short-Term (Expand Test Coverage)

- [ ] Test member assignment on LHS: `P.X = value`
- [ ] Test all numeric types as UDT fields (BYTE, SHORT, LONG)
- [ ] Test mixed-type UDTs (int + double + string)
- [ ] Test REDIM with UDT arrays
- [ ] Test ERASE with UDT arrays (string cleanup)

### Medium-Term (New Features)

- [ ] Passing UDTs to SUB/FUNCTION
- [ ] Returning UDTs from FUNCTION
- [ ] Allow keywords as field names in TYPE blocks
- [ ] Add `SIZEOF(TypeName)` operator
- [ ] Add alignment control (e.g., `PACKED` attribute)

### Long-Term (Advanced Features)

- [ ] Array fields in UDTs (if needed)
- [ ] Methods in UDTs (SUB/FUNCTION inside TYPE)
- [ ] Operator overloading for UDTs
- [ ] Generic/template UDTs

---

## 13. Documentation Status

### Existing Docs

- ✅ `UDT_TEST_RESULTS.md` - Test suite results and known issues
- ✅ `UserDefinedTypes.md` - High-level architecture overview
- ✅ `CONSTANTS_IMPLEMENTATION.md` - Mentions UDT integration
- ⚠️  No comprehensive UDT user guide

### Missing Documentation

- [ ] **User Guide**: How to use TYPE...END TYPE
- [ ] **API Reference**: UDT-related functions and operators
- [ ] **Codegen Internals**: Field offset calculation, memory layout
- [ ] **Migration Guide**: Porting UDTs from QB/FreeBASIC

---

## 14. Conclusion

FasterBASIC's UDT implementation is **substantially complete** and architecturally sound. The core infrastructure—from parsing through code generation—is in place and working for the most common use cases.

**The two blocking bugs are fixable in 3-5 hours of focused work:**
1. String field type inference (simple logic fix)
2. UDT array descriptor initialization (requires understanding array runtime)

Once these bugs are fixed, FasterBASIC will have **production-ready UDT support** comparable to or exceeding other BASIC compilers in its class.

**Confidence Level:** 🟢 High - The architecture is correct, tests are in place, only bug fixes needed.

---

## Appendix A: Code Locations Quick Reference

| Feature | File | Function/Line |
|---------|------|---------------|
| **Parser** | `fasterbasic_parser.cpp` | `parseTypeDeclarationStatement()` L2829 |
| **Semantic Analysis** | `fasterbasic_semantic.cpp` | `processTypeDeclarationStatement()` L449 |
| **Variable Declaration** | `fasterbasic_semantic.cpp` | `processDimStatement()` L843 |
| **Member Access Codegen** | `qbe_codegen_expressions.cpp` | `emitMemberAccessExpr()` L1899 |
| **Field Offset** | `qbe_codegen_helpers.cpp` | `calculateFieldOffset()` |
| **Type Inference (BROKEN)** | `fasterbasic_semantic.cpp` | `inferMemberAccessType()` L2415 |
| **Array Init (BROKEN)** | `qbe_codegen_statements.cpp` | `emitDim()` |
| **Type Symbols** | `fasterbasic_semantic.h` | `struct TypeSymbol` L697 |
| **AST Node** | `fasterbasic_ast.h` | `class MemberAccessExpression` L418 |

---

## Appendix B: Test File Summary

```basic
' test_udt_simple.bas (9 lines) ✅
TYPE Point
  X AS INTEGER
END TYPE
DIM P AS Point
P.X = 42
PRINT P.X

' test_udt_twofields.bas (11 lines) ✅
TYPE Point
  X AS INTEGER
  Y AS DOUBLE
END TYPE
DIM P AS Point
P.X = 10
P.Y = 20.5
PRINT P.X, P.Y

' test_udt_nested.bas (12 lines) ✅
TYPE Inner
  Value AS INTEGER
END TYPE
TYPE Outer
  Item AS Inner
END TYPE
DIM O AS Outer
O.Item.Value = 99
PRINT O.Item.Value

' test_udt_string.bas (11 lines) ❌
TYPE Person
  Name AS STRING
  Age AS INTEGER
END TYPE
DIM P AS Person
P.Name = "Alice"    ' ← BUG: Type mismatch
P.Age = 25
PRINT P.Name, P.Age

' test_udt_array.bas (14 lines) ❌
TYPE Point
  X AS INTEGER
  Y AS INTEGER
END TYPE
DIM Points(5) AS Point
Points(0).X = 100   ' ← BUG: Bounds corruption
Points(0).Y = 200
PRINT Points(0).X, Points(0).Y
```

---

**End of Report**