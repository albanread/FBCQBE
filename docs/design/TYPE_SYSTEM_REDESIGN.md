# FasterBASIC Type System Redesign

## Executive Summary

The current type system uses a simple `VariableType` enum that cannot distinguish between scalars and arrays, or between different user-defined types. This document proposes a bitmap-based type descriptor system that enables precise type tracking for:

- Scalars vs arrays
- Different user-defined types (UDTs)
- Future extensions (pointers, const, etc.)

## Current Problems

### 1. No Distinction Between Scalar and Array Types

```cpp
// Current system:
VariableSymbol var;  // Has VariableType::INT
ArraySymbol arr;     // Also has VariableType::INT

// Problem: These are stored in separate tables and can't be unified
// Cannot express "array of integers" as a type
```

### 2. USER_DEFINED Type is Not Specific

```cpp
// Current:
struct Point { x as double, y as double }
struct Sprite { x as double, y as double, w as double, h as double }

// Both would be VariableType::USER_DEFINED
// No way to distinguish Point from Sprite in type checking!
```

### 3. Weak Type Checking

```cpp
// Currently typeName is just a string stored separately
VariableSymbol var;
var.type = VariableType::USER_DEFINED;
var.typeName = "Point";  // String comparison, no strong typing
```

### 4. QBE Exposes Our Weaknesses

QBE performs strong type checking on its SSA instructions. When we generate incorrect types (e.g., treating an array as a scalar), QBE catches it. We need semantic analysis that is equally rigorous.

---

## Proposed Solution: TypeDescriptor System

### Design Principles

1. **Composable Types**: Use bit flags to compose type attributes
2. **Unique Type IDs**: Each UDT gets a unique numeric identifier
3. **Single Source of Truth**: All type information in one place
4. **Extensible**: Easy to add new attributes (const, pointer, byref, etc.)

### Core Type Descriptor Structure

```cpp
// Type attribute flags (can be combined)
enum class TypeAttribute : uint32_t {
    NONE        = 0x0000,
    IS_ARRAY    = 0x0001,  // This is an array type
    IS_POINTER  = 0x0002,  // Future: pointer type
    IS_CONST    = 0x0004,  // Future: const/readonly
    IS_BYREF    = 0x0008,  // Future: passed by reference
    IS_OPTIONAL = 0x0010,  // Future: optional parameter
    IS_VARIADIC = 0x0020,  // Future: variadic function
};

// Base type categories
enum class BaseType : uint8_t {
    UNKNOWN = 0,
    VOID,           // No type (SUB return)
    INTEGER,        // 64-bit signed integer (%)
    SINGLE,         // 32-bit float (!)
    DOUBLE,         // 64-bit float (#)
    STRING,         // Byte string ($)
    UNICODE,        // UTF-32 string ($) in OPTION UNICODE mode
    USER_DEFINED,   // Custom TYPE...END TYPE (use udtTypeId)
};

// Complete type descriptor
struct TypeDescriptor {
    BaseType baseType;
    uint32_t attributes;        // Bitfield of TypeAttribute flags
    uint32_t udtTypeId;         // For USER_DEFINED: unique type ID (0 = none)
    std::vector<int> arrayDims; // For arrays: dimension sizes (-1 = dynamic)
    
    // Constructors
    TypeDescriptor() 
        : baseType(BaseType::UNKNOWN), attributes(0), udtTypeId(0) {}
    
    TypeDescriptor(BaseType base) 
        : baseType(base), attributes(0), udtTypeId(0) {}
    
    TypeDescriptor(BaseType base, uint32_t attrs) 
        : baseType(base), attributes(attrs), udtTypeId(0) {}
    
    TypeDescriptor(uint32_t udtId) 
        : baseType(BaseType::USER_DEFINED), attributes(0), udtTypeId(udtId) {}
    
    // Query methods
    bool isArray() const { 
        return (attributes & (uint32_t)TypeAttribute::IS_ARRAY) != 0; 
    }
    
    bool isPointer() const { 
        return (attributes & (uint32_t)TypeAttribute::IS_POINTER) != 0; 
    }
    
    bool isConst() const { 
        return (attributes & (uint32_t)TypeAttribute::IS_CONST) != 0; 
    }
    
    bool isByRef() const { 
        return (attributes & (uint32_t)TypeAttribute::IS_BYREF) != 0; 
    }
    
    bool isUserDefined() const { 
        return baseType == BaseType::USER_DEFINED; 
    }
    
    bool isNumeric() const {
        return baseType == BaseType::INTEGER || 
               baseType == BaseType::SINGLE || 
               baseType == BaseType::DOUBLE;
    }
    
    bool isString() const {
        return baseType == BaseType::STRING || 
               baseType == BaseType::UNICODE;
    }
    
    // Get the element type for an array
    TypeDescriptor elementType() const {
        if (!isArray()) return *this;
        
        TypeDescriptor elem = *this;
        elem.attributes &= ~(uint32_t)TypeAttribute::IS_ARRAY;
        elem.arrayDims.clear();
        return elem;
    }
    
    // Make an array version of this type
    TypeDescriptor asArray(const std::vector<int>& dims) const {
        TypeDescriptor arr = *this;
        arr.attributes |= (uint32_t)TypeAttribute::IS_ARRAY;
        arr.arrayDims = dims;
        return arr;
    }
    
    // Equality comparison
    bool operator==(const TypeDescriptor& other) const {
        if (baseType != other.baseType) return false;
        if (attributes != other.attributes) return false;
        if (isUserDefined() && udtTypeId != other.udtTypeId) return false;
        // Note: arrayDims don't affect type equality (structural vs nominal)
        return true;
    }
    
    bool operator!=(const TypeDescriptor& other) const {
        return !(*this == other);
    }
    
    // String representation for debugging/error messages
    std::string toString() const;
};
```

### Enhanced Type Symbol

```cpp
struct TypeSymbol {
    struct Field {
        std::string name;
        TypeDescriptor type;  // Now uses TypeDescriptor instead of VariableType!
        size_t offset;        // Byte offset in the struct (for codegen)
        
        Field(const std::string& n, const TypeDescriptor& t, size_t off = 0)
            : name(n), type(t), offset(off) {}
    };
    
    std::string name;
    uint32_t typeId;                         // UNIQUE identifier for this type
    std::vector<Field> fields;
    SourceLocation declaration;
    bool isDeclared;
    
    // SIMD optimization hints
    TypeDeclarationStatement::SIMDType simdType;
    
    TypeSymbol() 
        : typeId(0), isDeclared(false), 
          simdType(TypeDeclarationStatement::SIMDType::NONE) {}
    
    TypeSymbol(const std::string& n, uint32_t id) 
        : name(n), typeId(id), isDeclared(true),
          simdType(TypeDeclarationStatement::SIMDType::NONE) {}
    
    const Field* findField(const std::string& fieldName) const;
    size_t getSizeInBytes() const;  // Calculate total size
    std::string toString() const;
};
```

### Unified Symbol Structures

```cpp
// Variable (scalar or array - unified!)
struct VariableSymbol {
    std::string name;
    TypeDescriptor type;             // Complete type information
    bool isDeclared;                 // Explicit vs implicit
    bool isUsed;
    SourceLocation firstUse;
    std::string functionScope;       // Empty = global
    
    VariableSymbol()
        : type(BaseType::UNKNOWN), isDeclared(false), 
          isUsed(false), functionScope("") {}
    
    std::string toString() const;
};

// Arrays are now just variables with IS_ARRAY attribute
// But we can keep ArraySymbol for backward compatibility during transition:
struct ArraySymbol {
    std::string name;
    TypeDescriptor type;             // Will have IS_ARRAY attribute set
    bool isDeclared;
    SourceLocation declaration;
    int totalSize;
    std::string functionScope;
    
    ArraySymbol()
        : isDeclared(false), totalSize(0), functionScope("") {
        type.attributes |= (uint32_t)TypeAttribute::IS_ARRAY;
    }
    
    std::string toString() const;
};

// Function/Sub signature
struct FunctionSymbol {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<TypeDescriptor> parameterTypes;  // Now TypeDescriptor!
    std::vector<bool> parameterIsByRef;
    TypeDescriptor returnType;                   // Now TypeDescriptor!
    SourceLocation definition;
    const Expression* body;
    
    FunctionSymbol() : body(nullptr) {}
    
    std::string toString() const;
};
```

### Enhanced Symbol Table

```cpp
struct SymbolTable {
    // Symbol storage
    std::unordered_map<std::string, VariableSymbol> variables;
    std::unordered_map<std::string, ArraySymbol> arrays;  // May merge with variables
    std::unordered_map<std::string, FunctionSymbol> functions;
    std::unordered_map<std::string, TypeSymbol> types;
    std::unordered_map<int, LineNumberSymbol> lineNumbers;
    std::unordered_map<std::string, LabelSymbol> labels;
    std::unordered_map<std::string, ConstantSymbol> constants;
    DataSegment dataSegment;
    
    // Type registry
    uint32_t nextTypeId;                             // Auto-increment type IDs
    std::unordered_map<uint32_t, TypeSymbol*> typeRegistry;  // ID -> TypeSymbol
    std::unordered_map<std::string, uint32_t> typeNameToId;  // Name -> ID
    
    // Configuration
    int nextLabelId;
    int arrayBase;
    bool unicodeMode;
    bool errorTracking;
    bool cancellableLoops;
    bool eventsUsed;
    bool forceYieldEnabled;
    int forceYieldBudget;
    
    SymbolTable() 
        : nextTypeId(1), nextLabelId(0), arrayBase(0), 
          unicodeMode(false), errorTracking(false), 
          cancellableLoops(false), eventsUsed(false), 
          forceYieldEnabled(false), forceYieldBudget(0) {}
    
    // Type registry methods
    uint32_t registerType(const std::string& name, TypeSymbol& typeSym);
    TypeSymbol* lookupTypeById(uint32_t typeId);
    uint32_t getTypeId(const std::string& name) const;
    
    std::string toString() const;
};
```

---

## Migration Strategy

### Phase 1: Add TypeDescriptor (Week 1)

1. Add `TypeDescriptor` struct to `fasterbasic_semantic.h`
2. Add type registry to `SymbolTable`
3. Keep existing `VariableType` enum for backward compatibility
4. Add conversion functions between old and new systems

```cpp
// Conversion helpers during migration
TypeDescriptor typeDescriptorFromLegacy(VariableType oldType, bool isArray = false);
VariableType legacyTypeFromDescriptor(const TypeDescriptor& desc);
```

### Phase 2: Convert Core Symbols (Week 2)

1. Update `TypeSymbol::Field` to use `TypeDescriptor`
2. Update `FunctionSymbol` parameter and return types
3. Update type inference methods to return `TypeDescriptor`
4. Update type checking to use `TypeDescriptor` comparisons

### Phase 3: Unify Variables and Arrays (Week 3)

1. Migrate `VariableSymbol` to use `TypeDescriptor`
2. Migrate `ArraySymbol` to use `TypeDescriptor`
3. Update all DIM statement processing
4. Update all type inference code paths

### Phase 4: Update Code Generation (Week 4)

1. Update QBE codegen to extract type info from `TypeDescriptor`
2. Update runtime calls to use proper type information
3. Test thoroughly with existing test suite
4. Add new tests for complex type scenarios

### Phase 5: Remove Legacy Code (Week 5)

1. Remove old `VariableType` enum
2. Remove conversion functions
3. Clean up any remaining legacy code
4. Update documentation

---

## Type Checking Improvements

### Precise Type Compatibility

```cpp
class SemanticAnalyzer {
    // Enhanced type checking
    bool isTypeCompatible(const TypeDescriptor& from, const TypeDescriptor& to,
                         bool allowImplicitConversion = true) const;
    
    bool canAssign(const TypeDescriptor& target, const TypeDescriptor& value) const;
    
    TypeDescriptor promoteTypes(const TypeDescriptor& left, const TypeDescriptor& right) const;
    
    // Check if types match exactly (no conversion)
    bool isExactMatch(const TypeDescriptor& a, const TypeDescriptor& b) const {
        return a == b;
    }
    
    // Check if value can be coerced to target type
    bool canCoerce(const TypeDescriptor& from, const TypeDescriptor& to) const;
};
```

### Example Type Checking Rules

```cpp
bool SemanticAnalyzer::isTypeCompatible(const TypeDescriptor& from, 
                                       const TypeDescriptor& to,
                                       bool allowImplicitConversion) const {
    // Exact match
    if (from == to) return true;
    
    // Array vs scalar mismatch
    if (from.isArray() != to.isArray()) return false;
    
    // UDT must match exactly
    if (from.isUserDefined() || to.isUserDefined()) {
        return from.udtTypeId == to.udtTypeId;
    }
    
    // String/numeric category mismatch
    if (from.isString() != to.isString()) return false;
    
    if (!allowImplicitConversion) return false;
    
    // Numeric promotions allowed
    if (from.isNumeric() && to.isNumeric()) {
        // INT -> SINGLE -> DOUBLE
        int fromRank = (from.baseType == BaseType::INTEGER) ? 0 :
                      (from.baseType == BaseType::SINGLE) ? 1 : 2;
        int toRank = (to.baseType == BaseType::INTEGER) ? 0 :
                    (to.baseType == BaseType::SINGLE) ? 1 : 2;
        return fromRank <= toRank;  // Can promote to higher precision
    }
    
    // STRING <-> UNICODE allowed in some contexts
    if (from.isString() && to.isString()) {
        return true;
    }
    
    return false;
}
```

---

## Usage Examples

### Example 1: Scalar Variables

```basic
' FasterBASIC code
DIM x AS INTEGER
DIM y AS DOUBLE
DIM name AS STRING
```

```cpp
// Semantic analyzer creates:
VariableSymbol x;
x.name = "x";
x.type = TypeDescriptor(BaseType::INTEGER);
x.isDeclared = true;

VariableSymbol y;
y.name = "y";
y.type = TypeDescriptor(BaseType::DOUBLE);
y.isDeclared = true;

VariableSymbol name;
name.name = "name";
name.type = TypeDescriptor(BaseType::STRING);
name.isDeclared = true;
```

### Example 2: Arrays

```basic
' FasterBASIC code
DIM numbers(10) AS INTEGER
DIM matrix(5, 5) AS DOUBLE
```

```cpp
// Semantic analyzer creates:
ArraySymbol numbers;
numbers.name = "numbers";
numbers.type = TypeDescriptor(BaseType::INTEGER).asArray({10});
numbers.isDeclared = true;

ArraySymbol matrix;
matrix.name = "matrix";
matrix.type = TypeDescriptor(BaseType::DOUBLE).asArray({5, 5});
matrix.isDeclared = true;
```

### Example 3: User-Defined Types

```basic
' FasterBASIC code
TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

TYPE Sprite
    pos AS Point      ' Nested UDT!
    width AS INTEGER
    height AS INTEGER
END TYPE

DIM p AS Point
DIM sprites(100) AS Sprite
```

```cpp
// Semantic analyzer creates:

// Type registration
TypeSymbol pointType("Point", 1);  // typeId = 1
pointType.fields.push_back(
    TypeSymbol::Field("x", TypeDescriptor(BaseType::DOUBLE), 0)
);
pointType.fields.push_back(
    TypeSymbol::Field("y", TypeDescriptor(BaseType::DOUBLE), 8)
);
symbolTable.registerType("Point", pointType);

TypeSymbol spriteType("Sprite", 2);  // typeId = 2
spriteType.fields.push_back(
    TypeSymbol::Field("pos", TypeDescriptor(1), 0)  // UDT ref by ID!
);
spriteType.fields.push_back(
    TypeSymbol::Field("width", TypeDescriptor(BaseType::INTEGER), 16)
);
spriteType.fields.push_back(
    TypeSymbol::Field("height", TypeDescriptor(BaseType::INTEGER), 24)
);
symbolTable.registerType("Sprite", spriteType);

// Variable declarations
VariableSymbol p;
p.name = "p";
p.type = TypeDescriptor(1);  // typeId for Point
p.isDeclared = true;

ArraySymbol sprites;
sprites.name = "sprites";
sprites.type = TypeDescriptor(2).asArray({100});  // Array of Sprite (typeId 2)
sprites.isDeclared = true;
```

### Example 4: Function Parameters

```basic
' FasterBASIC code
FUNCTION Distance(p1 AS Point, p2 AS Point) AS DOUBLE
    Distance = SQR((p2.x - p1.x)^2 + (p2.y - p1.y)^2)
END FUNCTION

SUB UpdateSprite(BYREF s AS Sprite, dx AS INTEGER, dy AS INTEGER)
    s.pos.x = s.pos.x + dx
    s.pos.y = s.pos.y + dy
END SUB
```

```cpp
// Function symbols:
FunctionSymbol distFunc;
distFunc.name = "Distance";
distFunc.parameters = {"p1", "p2"};
distFunc.parameterTypes = {
    TypeDescriptor(1),  // Point (typeId 1)
    TypeDescriptor(1)   // Point (typeId 1)
};
distFunc.parameterIsByRef = {false, false};
distFunc.returnType = TypeDescriptor(BaseType::DOUBLE);

FunctionSymbol updateSub;
updateSub.name = "UpdateSprite";
updateSub.parameters = {"s", "dx", "dy"};
updateSub.parameterTypes = {
    TypeDescriptor(2) | TypeAttribute::IS_BYREF,  // BYREF Sprite
    TypeDescriptor(BaseType::INTEGER),
    TypeDescriptor(BaseType::INTEGER)
};
updateSub.parameterIsByRef = {true, false, false};
updateSub.returnType = TypeDescriptor(BaseType::VOID);
```

---

## Benefits

### 1. Precise Type Tracking

```cpp
// Can now distinguish:
TypeDescriptor intScalar(BaseType::INTEGER);
TypeDescriptor intArray = intScalar.asArray({10});

// Type checking catches errors:
if (!isTypeCompatible(intArray, intScalar)) {
    error("Cannot assign array to scalar!");
}
```

### 2. Strong UDT Typing

```cpp
// Different UDTs are incompatible:
TypeDescriptor point(1);   // Point, typeId=1
TypeDescriptor sprite(2);  // Sprite, typeId=2

if (point.udtTypeId != sprite.udtTypeId) {
    error("Cannot assign Sprite to Point!");
}
```

### 3. Better Error Messages

```cpp
// Old: "Type mismatch"
// New: "Cannot assign ARRAY OF INTEGER to INTEGER scalar"
// New: "Cannot assign type 'Sprite' to type 'Point'"
```

### 4. QBE-Compatible Type Information

```cpp
// Generate correct QBE types:
std::string TypeDescriptor::toQBEType() const {
    if (isArray()) {
        return "l";  // Arrays are pointers (long)
    }
    
    switch (baseType) {
        case BaseType::INTEGER: return "l";
        case BaseType::SINGLE: return "s";
        case BaseType::DOUBLE: return "d";
        case BaseType::STRING:
        case BaseType::UNICODE: return "l";  // Pointer to string descriptor
        case BaseType::USER_DEFINED: return "l";  // Pointer to struct
        default: return "w";
    }
}
```

### 5. Future Extensions

Easy to add:
- `IS_CONST` for const parameters/variables
- `IS_POINTER` for explicit pointer types
- `IS_OPTIONAL` for optional parameters
- Reference counting flags
- Nullable types

---

## Implementation Checklist

- [ ] Add `TypeDescriptor` struct to `fasterbasic_semantic.h`
- [ ] Add `BaseType` enum
- [ ] Add `TypeAttribute` flags
- [ ] Update `TypeSymbol` with `typeId` and `Field::type`
- [ ] Add type registry to `SymbolTable`
- [ ] Implement `registerType()` and lookup methods
- [ ] Add backward compatibility conversion functions
- [ ] Update `FunctionSymbol` to use `TypeDescriptor`
- [ ] Update type inference methods signatures
- [ ] Implement new type checking methods
- [ ] Update `processDimStatement()` for new types
- [ ] Update `processTypeDeclarationStatement()` for type IDs
- [ ] Update `inferExpressionType()` to return `TypeDescriptor`
- [ ] Update `checkTypeCompatibility()` for new system
- [ ] Update member access type inference
- [ ] Update array access type inference
- [ ] Migrate `VariableSymbol` to `TypeDescriptor`
- [ ] Migrate `ArraySymbol` to `TypeDescriptor`
- [ ] Update QBE codegen to use `TypeDescriptor`
- [ ] Add comprehensive test cases
- [ ] Update documentation
- [ ] Remove legacy `VariableType` enum

---

## Testing Strategy

### Unit Tests

1. **Type descriptor creation and queries**
   - Scalar types
   - Array types
   - UDT types
   - Type equality

2. **Type compatibility**
   - Numeric promotions
   - String conversions
   - UDT matching
   - Array vs scalar

3. **Symbol table operations**
   - Type registration
   - Type lookup by ID
   - Type lookup by name

### Integration Tests

1. **Variable declarations**
   - Scalar variables of all types
   - Array variables
   - UDT variables
   - Mixed scenarios

2. **Function parameters**
   - Pass by value
   - Pass by reference (BYREF)
   - UDT parameters
   - Array parameters

3. **Type checking**
   - Assignment compatibility
   - Function call argument matching
   - Binary operation type promotion
   - Member access on UDTs

### Regression Tests

Run entire existing test suite to ensure no breakage:
- `test_types.bas`
- `test_types_simple.bas`
- `test_udt_*.bas`
- `test_arrays_*.bas`
- All function/sub tests

---

## Performance Considerations

### Memory Impact

- `TypeDescriptor` is ~32 bytes (base + attrs + udtId + vector)
- Minimal increase compared to current `VariableType` enum (4 bytes)
- Type registry adds one map per program (negligible)

### Compilation Speed

- Type ID lookup is O(1) hash map
- Type comparison is O(1) (compare IDs, not strings)
- Should be faster than current string comparisons!

### Runtime Impact

- Zero runtime impact (type checking at compile time only)
- May improve codegen quality due to better type information

---

## Conclusion

This type system redesign provides:

1. **Precision**: Distinguish scalars, arrays, and different UDTs
2. **Safety**: Strong type checking catches more errors
3. **Clarity**: Better error messages for developers
4. **Extensibility**: Easy to add new type attributes
5. **QBE Compatibility**: Generate correct types for QBE

The migration can be done incrementally over 5 weeks with minimal disruption to existing code.

---

## References

- Current implementation: `fasterbasic_semantic.h`, `fasterbasic_semantic.cpp`
- QBE type system: `qbe/all.h`, `qbe/parse.c`
- Related docs: `UserDefinedTypes.md`, `COERCION_STRATEGY.md`
