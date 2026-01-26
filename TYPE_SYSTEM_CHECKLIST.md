# FasterBASIC Type System Redesign - Implementation Checklist

**Last Updated:** January 25, 2025

---

## Phase 1: Infrastructure ✅ COMPLETE

- [x] Define BaseType enum (20+ types)
- [x] Define TypeAttribute flags (8 attributes)
- [x] Implement TypeDescriptor struct
  - [x] Constructor overloads
  - [x] Type predicate methods (isInteger, isFloat, isNumeric, etc.)
  - [x] getBitWidth() method
  - [x] toQBEType() method
  - [x] toQBEMemOp() method
  - [x] toString() method
  - [x] Equality operators
- [x] Add type registry to SymbolTable
  - [x] typeNameToId map
  - [x] nextTypeId counter
  - [x] allocateTypeId() method
  - [x] getTypeId() method
- [x] Implement conversion helpers
  - [x] legacyTypeToDescriptor()
  - [x] descriptorToLegacyType()
  - [x] getTypeSuffix()
  - [x] baseTypeFromSuffix()
- [x] Validate compilation (0 errors)
- [x] Document Phase 1 completion

**Status:** ✅ 100% Complete (Jan 25, 2025)

---

## Phase 2: Type Inference & Coercion ✅ COMPLETE

### Type Inference Methods
- [x] inferExpressionTypeD()
  - [x] Integer literal magnitude inference
  - [x] String literal with UNICODE mode
  - [x] Delegate to specialized methods
- [x] inferBinaryExpressionTypeD()
  - [x] String concatenation with UNICODE promotion
  - [x] Comparison operators return INTEGER
  - [x] Logical operators return INTEGER
  - [x] Arithmetic uses type promotion
- [x] inferUnaryExpressionTypeD()
  - [x] NOT returns INTEGER
  - [x] Unary +/- preserves type
- [x] inferVariableTypeD()
  - [x] Function scope checking
  - [x] Symbol table lookup with UDT ID
  - [x] Name-based inference fallback
- [x] inferArrayAccessTypeD()
  - [x] Distinguish array vs function call
  - [x] UDT element type tracking
- [x] inferFunctionCallTypeD()
  - [x] User-defined function lookup
  - [x] Built-in function support
  - [x] UDT return type tracking
- [x] inferRegistryFunctionTypeD()
  - [x] Map ModularCommands::ReturnType
  - [x] UNICODE mode handling
- [x] inferMemberAccessTypeD()
  - [x] Base object type resolution
  - [x] Field type lookup
  - [x] Nested UDT support

### Coercion System
- [x] Define CoercionResult enum (5 levels)
- [x] Implement checkCoercion()
  - [x] Identical type handling
  - [x] String-to-string (STRING ↔ UNICODE)
  - [x] Numeric conversions
  - [x] String-numeric incompatibility
  - [x] UDT type safety
- [x] Implement checkNumericCoercion()
  - [x] Integer widening (IMPLICIT_SAFE)
  - [x] Integer narrowing (IMPLICIT_LOSSY)
  - [x] Integer to float (IMPLICIT_SAFE/LOSSY)
  - [x] Float to integer (EXPLICIT_REQUIRED)
  - [x] Float widening/narrowing
  - [x] Signed/unsigned handling
- [x] Implement validateAssignment()
  - [x] Warning for lossy conversions
  - [x] Error for explicit required
  - [x] Error for incompatible
  - [x] Suggest conversion functions
- [x] Implement promoteTypesD()
  - [x] DOUBLE highest priority
  - [x] SINGLE next priority
  - [x] Integer width-based promotion

### Type Inference Helpers
- [x] inferTypeFromSuffixD(TokenType)
- [x] inferTypeFromSuffixD(char)
- [x] inferTypeFromNameD(string)
  - [x] Normalized suffixes (_STRING, _INT, etc.)
  - [x] New suffixes (_LONG, _BYTE, _SHORT)
  - [x] Trailing suffix characters
  - [x] Default to DOUBLE

### Testing & Validation
- [x] Validate compilation (0 errors)
- [x] Verify backward compatibility
- [x] Document Phase 2 completion

**Status:** ✅ 100% Complete (Jan 25, 2025)

---

## Phase 3: Symbol Table Migration 🟡 NOT STARTED

### Symbol Structure Updates
- [ ] Update VariableSymbol
  - [ ] Change type field to TypeDescriptor
  - [ ] Remove typeName (now in TypeDescriptor)
  - [ ] Update constructor
  - [ ] Update toString()
- [ ] Update ArraySymbol
  - [ ] Change type to elementType (TypeDescriptor)
  - [ ] Remove asTypeName (now in TypeDescriptor)
  - [ ] Update constructor
  - [ ] Update toString()
- [ ] Update FunctionSymbol
  - [ ] Change parameterTypes to vector<TypeDescriptor>
  - [ ] Change returnType to TypeDescriptor
  - [ ] Remove parameterTypeNames (in TypeDescriptor)
  - [ ] Remove returnTypeName (in TypeDescriptor)
  - [ ] Update constructor
  - [ ] Update toString()
- [ ] Update TypeSymbol::Field
  - [ ] Change to TypeDescriptor type
  - [ ] Remove builtInType and isBuiltIn
  - [ ] Simplify to single type field
  - [ ] Update constructor

### Symbol Table Methods
- [ ] Update declareVariable() signature
- [ ] Update declareArray() signature
- [ ] Update declareFunction() signature
- [ ] Update lookupVariable() to return TypeDescriptor
- [ ] Update lookupArray() to return TypeDescriptor
- [ ] Update lookupFunction() to return TypeDescriptor
- [ ] Add legacy compatibility wrappers (temporary)

### Statement Processing Updates
- [ ] Update processDimStatement()
  - [ ] Populate TypeDescriptor from AS clause
  - [ ] Handle new type names (BYTE, SHORT, LONG)
  - [ ] Allocate UDT type IDs
- [ ] Update processFunctionStatement()
  - [ ] Parameter TypeDescriptor
  - [ ] Return TypeDescriptor
- [ ] Update processSubStatement()
  - [ ] Parameter TypeDescriptor
- [ ] Update processTypeDeclarationStatement()
  - [ ] Allocate UDT type ID
  - [ ] Field TypeDescriptors

### Validation & Testing
- [ ] Update all references to symbol.type
- [ ] Test variable declarations
- [ ] Test array declarations
- [ ] Test function declarations
- [ ] Test TYPE declarations
- [ ] Test UDT type ID uniqueness
- [ ] Validate compilation
- [ ] Document Phase 3 completion

**Status:** 🟡 Ready to begin

---

## Phase 4: Parser & Suffix Support 🟡 NOT STARTED

### Lexer Updates
- [ ] Add '@' token (BYTE suffix)
- [ ] Add '^' token (SHORT suffix)
- [ ] Test lexer with new suffixes

### Parser Updates
- [ ] Add AS BYTE to type parsing
- [ ] Add AS SHORT to type parsing
- [ ] Add AS LONG to type parsing
- [ ] Add AS UBYTE to type parsing
- [ ] Add AS USHORT to type parsing
- [ ] Add AS UINTEGER to type parsing
- [ ] Add AS ULONG to type parsing
- [ ] Add AS SINGLE (clarify vs FLOAT)

### Variable Declaration Parsing
- [ ] Parse DIM var@ AS BYTE
- [ ] Parse DIM var^ AS SHORT
- [ ] Parse DIM var& AS LONG
- [ ] Handle suffix priority vs AS clause

### Function/Sub Declaration Parsing
- [ ] Parse parameter types (new types)
- [ ] Parse return types (new types)
- [ ] FUNCTION name& AS LONG
- [ ] FUNCTION name() AS BYTE

### TYPE Declaration Parsing
- [ ] Parse fields with new types
- [ ] Field AS BYTE
- [ ] Field AS SHORT
- [ ] Field AS LONG
- [ ] Field AS unsigned types

### Validation & Testing
- [ ] Test all new suffix parsing
- [ ] Test all new AS type parsing
- [ ] Test mixed old/new syntax
- [ ] Validate compilation
- [ ] Document Phase 4 completion

**Status:** 🟡 Blocked on Phase 3

---

## Phase 5: Code Generator & Runtime 🟡 NOT STARTED

### Code Generator Core
- [ ] Replace VariableType with TypeDescriptor in codegen
- [ ] Use TypeDescriptor::toQBEType() for values
- [ ] Use TypeDescriptor::toQBEMemOp() for memory ops
- [ ] Update variable code generation
- [ ] Update expression code generation
- [ ] Update function call code generation

### QBE Type Emission
- [ ] Emit correct QBE types (w, l, s, d)
- [ ] Emit memory operations (sb, ub, sh, uh)
- [ ] Test BYTE sign extension (sb)
- [ ] Test UBYTE zero extension (ub)
- [ ] Test SHORT sign extension (sh)
- [ ] Test USHORT zero extension (uh)

### Array Support
- [ ] Implement array descriptor generation
  - [ ] Descriptor layout (base, dims, size)
  - [ ] Element type tracking
  - [ ] Index type selection (INTEGER vs LONG)
- [ ] Array element access codegen
- [ ] Multi-dimensional indexing
- [ ] Dynamic array (REDIM) support

### String Support
- [ ] Implement string descriptor generation
  - [ ] Descriptor layout (data, length, capacity)
  - [ ] STRING vs UNICODE handling
  - [ ] Index type (INTEGER vs LONG)
- [ ] String operations codegen
- [ ] String concatenation
- [ ] String comparison

### Loop Support
- [ ] Implement loop index promotion
  - [ ] User type → LOOP_INDEX (LONG)
  - [ ] Safe increment/comparison
  - [ ] Demotion back to user type
- [ ] FOR loop codegen with promotion
- [ ] FOR IN loop codegen
- [ ] WHILE/UNTIL loop codegen

### Runtime Support
- [ ] Update runtime type checking
- [ ] Add conversion functions if needed
  - [ ] CBYTE() - Convert to BYTE
  - [ ] CSHORT() - Convert to SHORT
  - [ ] Existing CINT(), CLNG(), CSNG(), CDBL()
- [ ] Update descriptor runtime functions
- [ ] Test runtime compatibility

### Validation & Testing
- [ ] Test simple programs with new types
- [ ] Test complex expressions
- [ ] Test arrays with new types
- [ ] Test functions with new types
- [ ] Test UDT operations
- [ ] Validate QBE output
- [ ] Run QBE compiler on output
- [ ] Test generated programs
- [ ] Document Phase 5 completion

**Status:** 🟡 Blocked on Phases 3 & 4

---

## Phase 6: Testing & Documentation 🟡 NOT STARTED

### Unit Tests
- [ ] TypeDescriptor operations
  - [ ] Constructor tests
  - [ ] Predicate tests (isInteger, etc.)
  - [ ] toQBEType() tests
  - [ ] toQBEMemOp() tests
  - [ ] Equality tests
- [ ] Coercion tests
  - [ ] checkCoercion() matrix
  - [ ] checkNumericCoercion() cases
  - [ ] validateAssignment() behavior
  - [ ] promoteTypesD() combinations
- [ ] Type inference tests
  - [ ] Integer literal magnitude
  - [ ] All expression types
  - [ ] All suffix types
  - [ ] Name-based inference
- [ ] Conversion helper tests
  - [ ] legacyTypeToDescriptor()
  - [ ] descriptorToLegacyType()
  - [ ] Suffix conversion

### Integration Tests
- [ ] Type combinations
  - [ ] All numeric type pairs
  - [ ] Numeric with string
  - [ ] UDT with primitives
- [ ] Complex expressions
  - [ ] Nested expressions
  - [ ] Mixed types
  - [ ] Type promotion chains
- [ ] Symbol table
  - [ ] Variable declarations
  - [ ] Array declarations
  - [ ] Function declarations
  - [ ] TYPE declarations
  - [ ] UDT type ID uniqueness
- [ ] Code generation
  - [ ] QBE output validation
  - [ ] Runtime correctness
  - [ ] Performance tests

### Test Programs
- [ ] Simple type tests (all types)
- [ ] Arithmetic operations
- [ ] String operations
- [ ] Array operations
- [ ] Function tests
- [ ] UDT tests
- [ ] Loop tests
- [ ] Complex real-world programs

### Performance Testing
- [ ] Benchmark compilation time
- [ ] Benchmark runtime performance
- [ ] Compare with legacy system
- [ ] Profile hot paths
- [ ] Optimize if needed (target: < 5% overhead)

### Documentation
- [ ] User documentation
  - [ ] New type reference
  - [ ] Type suffix guide (updated)
  - [ ] Coercion rules explanation
  - [ ] Conversion function reference
  - [ ] Migration guide
- [ ] Developer documentation
  - [ ] Architecture overview
  - [ ] TypeDescriptor API reference
  - [ ] Codegen guide
  - [ ] Extending type system
- [ ] Language reference updates
  - [ ] Data types section
  - [ ] Type conversion section
  - [ ] Examples and tutorials
- [ ] Release notes
  - [ ] New features
  - [ ] Breaking changes (if any)
  - [ ] Migration steps

### Final Validation
- [ ] All tests passing
- [ ] No regressions
- [ ] Performance acceptable
- [ ] Documentation complete
- [ ] Code review complete
- [ ] Ready for release

**Status:** 🟡 Blocked on Phases 3-5

---

## Summary Status

| Phase | Status | Completion | Estimated Time | Actual Time |
|-------|--------|------------|----------------|-------------|
| Phase 1 | ✅ Complete | 100% | 2-3 days | ~4 hours |
| Phase 2 | ✅ Complete | 100% | 2-3 days | ~3 hours |
| Phase 3 | 🟡 Not Started | 0% | 2-3 days | - |
| Phase 4 | 🟡 Not Started | 0% | 2-3 days | - |
| Phase 5 | 🟡 Not Started | 0% | 3-4 days | - |
| Phase 6 | 🟡 Not Started | 0% | 2-3 days | - |
| **Overall** | **🟢 On Track** | **33%** | **13-18 days** | **~7 hours** |

---

## Quick Reference

### Completed
- ✅ TypeDescriptor infrastructure
- ✅ Type inference (all methods)
- ✅ Coercion checking (5-level system)
- ✅ Type promotion rules
- ✅ QBE type mapping
- ✅ Backward compatibility maintained

### Next Up (Phase 3)
- Update VariableSymbol
- Update ArraySymbol
- Update FunctionSymbol
- Update TypeSymbol::Field
- Migrate statement processing

### Future Work
- Parser updates (Phase 4)
- Codegen updates (Phase 5)
- Testing & docs (Phase 6)

---

**Last Updated:** January 25, 2025  
**Next Review:** After Phase 3 completion