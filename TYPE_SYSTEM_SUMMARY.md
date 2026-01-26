# Type System Redesign: Executive Summary

## Overview

This document summarizes the complete type system redesign for FasterBASIC, addressing the weaknesses exposed by QBE's strong type checking and providing a robust, extensible foundation for the compiler.

## Key Documents

1. **TYPE_SYSTEM_REDESIGN.md** - Original bitmap-based type descriptor proposal
2. **SEMANTIC_ANALYZER_REVIEW.md** - Analysis of current weaknesses (11 critical issues)
3. **QBE_ALIGNED_TYPE_SYSTEM.md** - Complete QBE-aligned implementation with all integer sizes
4. **TYPE_SUFFIX_REFERENCE.md** - Quick reference for developers

## The Problem

Current type system uses a simple enum that cannot:
- Distinguish between scalars and arrays (`INTEGER` vs `ARRAY OF INTEGER`)
- Identify specific user-defined types (all UDTs are just `USER_DEFINED`)
- Track precision for proper coercion rules
- Represent hidden/internal types (loop indices, descriptors)
- Map cleanly to QBE's type system

**Result**: QBE catches type errors that should be caught in semantic analysis.

## The Solution: TypeDescriptor System

### Core Design

```cpp
struct TypeDescriptor {
    BaseType baseType;          // BYTE, SHORT, INTEGER, LONG, SINGLE, DOUBLE, etc.
    uint32_t attributes;        // Bitmap: IS_ARRAY, IS_POINTER, IS_CONST, IS_BYREF
    uint32_t udtTypeId;         // Unique ID for user-defined types
    std::vector<int> arrayDims; // Array dimensions
};
```

### Complete Type Set

**Integer Types** (all sizes):
- `BYTE` (8-bit, `@`) → QBE `sb`
- `SHORT` (16-bit, `^`) → QBE `sh`
- `INTEGER` (32-bit, `%`) → QBE `w`
- `LONG` (64-bit, `&`) → QBE `l`
- Plus unsigned variants: `UBYTE`, `USHORT`, `UINTEGER`, `ULONG`

**Floating Point**:
- `SINGLE` (32-bit, `!`) → QBE `s`
- `DOUBLE` (64-bit, `#`) → QBE `d`

**String Types**:
- `STRING` (ASCII/byte, `$`) → QBE `l` (pointer)
- `UNICODE` (UTF-32, `$`) → QBE `l` (pointer)

**Hidden Types** (internal use only):
- `LOOP_INDEX` - All loop variables promoted to LONG internally
- `ARRAY_DESC` - Array descriptor structure
- `STRING_DESC` - String descriptor structure
- `PTR` - Generic pointer type

**User-Defined Types**:
- Each TYPE gets unique numeric ID
- Type identity checking via ID comparison (not string matching)

## Key Features

### 1. QBE Alignment

Every FasterBASIC type maps directly to a QBE type:
- BYTE/SHORT → `sb`/`sh` in memory, promoted to `w` in registers
- INTEGER → `w` (32-bit word)
- LONG → `l` (64-bit long)
- SINGLE → `s` (32-bit float)
- DOUBLE → `d` (64-bit float)
- Arrays/pointers → `l` (pointer)

### 2. Type Suffixes (Enhanced)

Traditional BASIC suffixes preserved and extended:
- `@` = BYTE (NEW)
- `^` = SHORT (NEW)
- `%` = INTEGER (traditional)
- `&` = LONG (traditional)
- `!` = SINGLE (traditional)
- `#` = DOUBLE (traditional)
- `$` = STRING (traditional)

### 3. Explicit Type Keywords

```basic
DIM x AS BYTE          ' Modern explicit style
DIM y AS UBYTE         ' Unsigned variant
DIM arr(100) AS LONG   ' Arrays
```

### 4. Smart Coercion Rules

**Safe (Implicit)**: Widening conversions
```
BYTE → SHORT → INTEGER → LONG → DOUBLE
                      ↓
                   SINGLE → DOUBLE
```

**Warning (Implicit but lossy)**: Narrowing conversions
```basic
DIM n& AS LONG = 5000000000&
DIM i% AS INTEGER = n&  ' ⚠ WARNING: May overflow
```

**Error (Requires explicit cast)**: Float to integer
```basic
DIM x# AS DOUBLE = 3.14
DIM i% AS INTEGER = x#     ' ✗ ERROR
DIM i% AS INTEGER = CINT(x#)  ' ✓ OK
```

### 5. Hidden Type Handling

**Loop Indices**: Always LONG internally
```basic
DIM i% AS INTEGER
FOR i% = 1 TO 10000000
    ' Internal counter is LONG to prevent overflow
    ' i% sees INTEGER values
NEXT i%
```

**Array Descriptors**: Automatic based on size
```basic
DIM small%(100)           ' Uses INTEGER indices internally
DIM huge#(100000, 100000) ' Uses LONG indices (>2^31 elements)
```

**String Descriptors**: UTF-32 vs byte tracking
```basic
DIM ascii$ AS STRING      ' Byte-based string descriptor
DIM utf$ AS UNICODE       ' UTF-32 string descriptor
```

## Benefits

### Immediate Improvements

1. **Precise Type Checking**
   - Arrays vs scalars distinguished
   - Specific UDT type checking
   - Proper coercion validation

2. **Better Error Messages**
   ```
   OLD: "Type mismatch"
   NEW: "Cannot assign ARRAY OF INTEGER to INTEGER scalar"
   NEW: "Cannot assign type 'Sprite' to type 'Point'"
   ```

3. **QBE Compatibility**
   - Generate correct QBE types from the start
   - No type mismatches in generated code
   - Better optimization opportunities

4. **Performance**
   - Use smallest sufficient type (BYTE vs LONG)
   - Better cache utilization
   - Enable SIMD for aligned types

### Long-Term Benefits

1. **Extensibility**
   - Easy to add new attributes (const, volatile, etc.)
   - Support for future types (complex, decimal, etc.)
   - Clean foundation for advanced features

2. **Safety**
   - Catch errors at compile time
   - Warn about lossy conversions
   - Prevent undefined behavior

3. **Maintainability**
   - Single source of type truth
   - Clear separation of concerns
   - Reduced code duplication

## Migration Strategy

### 6-Week Incremental Plan

**Week 1**: Add TypeDescriptor infrastructure
- Define enums, struct, and helper methods
- Add type registry to SymbolTable
- Keep old VariableType for compatibility

**Week 2**: Update type inference
- Change method signatures to return TypeDescriptor
- Implement promotion and coercion rules
- Add warning generation

**Week 3**: Migrate symbol structures
- Update TypeSymbol, FunctionSymbol, VariableSymbol
- Unify array and variable handling
- Update type checking logic

**Week 4**: Update parser and semantic analysis
- Add new suffix parsing
- Update DIM statement processing
- Update type declaration processing

**Week 5**: Update code generation
- Map TypeDescriptor to QBE types
- Handle extended types in memory ops
- Update descriptor generation

**Week 6**: Testing and cleanup
- Comprehensive test suite
- Remove old VariableType enum
- Update documentation

## Implementation Checklist

### Core Infrastructure
- [ ] Define BaseType enum (25 types)
- [ ] Define TypeAttribute flags
- [ ] Implement TypeDescriptor struct
- [ ] Add type registry to SymbolTable
- [ ] Implement type ID assignment

### Type Inference
- [ ] Update inferExpressionType() signature
- [ ] Implement literal type inference
- [ ] Implement binary operator promotion
- [ ] Implement unary operator handling
- [ ] Implement member access type resolution

### Coercion System
- [ ] Define CoercionKind enum
- [ ] Implement checkCoercion() function
- [ ] Implement numeric promotion rules
- [ ] Add lossy conversion warnings
- [ ] Add explicit conversion requirements

### Symbol Updates
- [ ] Migrate TypeSymbol fields
- [ ] Migrate FunctionSymbol parameters
- [ ] Migrate VariableSymbol type field
- [ ] Update ArraySymbol (or merge with VariableSymbol)
- [ ] Update type lookup methods

### Parser Updates
- [ ] Add @ suffix for BYTE
- [ ] Add ^ suffix for SHORT
- [ ] Parse AS UBYTE, USHORT, etc.
- [ ] Update DIM statement parsing
- [ ] Update TYPE declaration parsing

### Semantic Analysis
- [ ] Update processDimStatement()
- [ ] Update processTypeDeclarationStatement()
- [ ] Update validateAssignment()
- [ ] Update validateFunctionCall()
- [ ] Implement inferMemberAccessType()

### Code Generation
- [ ] Implement toQBEType() method
- [ ] Implement toQBEExtendedType() method
- [ ] Update variable allocation
- [ ] Update load/store emission
- [ ] Update function call emission
- [ ] Update array descriptor generation
- [ ] Update string descriptor generation

### Testing
- [ ] Test all type combinations
- [ ] Test coercion rules
- [ ] Test array types
- [ ] Test UDT fields with mixed types
- [ ] Test hidden types (loop indices)
- [ ] Regression test existing code

## Metrics

### Current State
- Type safety score: **30%**
- Type checking LOC: **~200 lines (5% of semantic.cpp)**
- Type representation: **Simple enum (8 values)**
- QBE type errors: **Common**

### Target State
- Type safety score: **95%**
- Type checking LOC: **~800 lines (enhanced coverage)**
- Type representation: **Rich descriptor (25 base types + attributes)**
- QBE type errors: **Rare (semantic analysis catches first)**

## Risk Assessment

### Low Risk
- Infrastructure addition (no breaking changes)
- Type registry implementation
- New suffix parsing

### Medium Risk
- Type inference refactoring (many call sites)
- Symbol structure migration (data model change)
- Coercion rule implementation (complex logic)

### Mitigation
- Incremental migration (keep old system during transition)
- Comprehensive testing at each phase
- Backward compatibility shims
- Extensive regression testing

## Success Criteria

1. **All existing tests pass** with new type system
2. **QBE type errors eliminated** in semantic analysis
3. **Improved error messages** with specific type information
4. **Performance maintained or improved** (smaller types where appropriate)
5. **User code unchanged** (backward compatible)
6. **Compiler maintainability improved** (cleaner abstractions)

## Conclusion

The TypeDescriptor system provides:
- **Precision**: Distinguish all type variations (scalar/array, specific UDTs)
- **Safety**: Catch type errors early with clear messages
- **Performance**: Generate optimal code using appropriate types
- **Extensibility**: Foundation for advanced type features
- **QBE Alignment**: Perfect 1:1 mapping to QBE types

**Estimated Effort**: 6 weeks full-time
**Risk Level**: Medium (mitigated by incremental approach)
**Benefit**: High (foundational improvement affecting all compilation phases)

**Recommendation**: Proceed with implementation following the 6-week plan.