# QBE-Aligned Type System for FasterBASIC

## Executive Summary

This document proposes a comprehensive type system for FasterBASIC that directly maps to QBE's type system while supporting BASIC's traditional type suffixes and modern explicit type declarations. The system includes:

1. **QBE Base Types**: BYTE, SHORT, WORD (INTEGER), LONG, SINGLE, DOUBLE
2. **User-Defined Types** with unique IDs
3. **Array Types** with dimension tracking
4. **Hidden/Internal Types** for loop indices, array descriptors, string descriptors
5. **Complete Type Suffixes** for all integer and float types
6. **Clean Coercion Rules** with explicit lossy conversion warnings

---

## 1. QBE Type System Review

### QBE's Type Classes

From `qbe/all.h` and `qbe/doc/il.txt`:

```c
enum {
    Kx = -1,  // "top" class (internal use)
    Kw,       // word (32-bit integer)
    Kl,       // long (64-bit integer)
    Ks,       // single (32-bit float)
    Kd        // double (64-bit float)
};

// Extended types (used in aggregates and data definitions)
enum {
    Ksb = 4,  // signed byte (8-bit)
    Kub,      // unsigned byte (8-bit)
    Ksh,      // signed half (16-bit)
    Kuh,      // unsigned half (16-bit)
    Kc,       // aggregate/composite type
    K0        // void
};
```

**Key Points**:
- Base types (w, l, s, d) are used for temporaries and computations
- Extended types (b, h) are used in aggregates and memory operations
- No explicit pointer type (pointers are `l` on 64-bit, `w` on 32-bit)
- All operations are on base types; extended types are sign/zero-extended on load

---

## 2. FasterBASIC Type System

### 2.1 Base Type Enumeration

```cpp
// Base scalar types matching QBE + BASIC conventions
enum class BaseType : uint8_t {
    UNKNOWN = 0,
    VOID,           // No type (SUB return, empty expressions)
    
    // Integer types (signed unless specified)
    BYTE,           // 8-bit signed integer    (-128 to 127)
    UBYTE,          // 8-bit unsigned integer  (0 to 255)
    SHORT,          // 16-bit signed integer   (-32768 to 32767)
    USHORT,         // 16-bit unsigned integer (0 to 65535)
    INTEGER,        // 32-bit signed integer   (Kw) - BASIC default integer
    UINTEGER,       // 32-bit unsigned integer (0 to 4.2G)
    LONG,           // 64-bit signed integer   (Kl) - for large arrays/64-bit
    ULONG,          // 64-bit unsigned integer (0 to 18.4E)
    
    // Floating point types
    SINGLE,         // 32-bit float (Ks) - BASIC default numeric
    DOUBLE,         // 64-bit float (Kd) - BASIC # suffix
    
    // String types
    STRING,         // ASCII/byte string (pointer to descriptor)
    UNICODE,        // UTF-32 string (pointer to descriptor)
    
    // User-defined types
    USER_DEFINED,   // Custom TYPE...END TYPE (use udtTypeId)
    
    // Hidden/internal types (not directly user-accessible)
    PTR,            // Generic pointer (Kl on 64-bit)
    ARRAY_DESC,     // Array descriptor structure
    STRING_DESC,    // String descriptor structure
    LOOP_INDEX,     // Loop index (always LONG for consistency)
};
```

### 2.2 Type Attributes (Bitmap Flags)

```cpp
enum class TypeAttribute : uint32_t {
    NONE        = 0x0000,
    
    // Storage modifiers
    IS_ARRAY    = 0x0001,  // This is an array type
    IS_POINTER  = 0x0002,  // Explicit pointer (for future use)
    IS_CONST    = 0x0004,  // Read-only / constant
    IS_BYREF    = 0x0008,  // Passed by reference (SUB/FUNCTION param)
    
    // Parameter attributes
    IS_OPTIONAL = 0x0010,  // Optional parameter (future)
    IS_VARIADIC = 0x0020,  // Variadic function (future)
    
    // Internal attributes
    IS_HIDDEN   = 0x0040,  // Hidden/internal type (not in user code)
    IS_UNSIGNED = 0x0080,  // Unsigned integer variant
    
    // Array-specific
    DYNAMIC_ARRAY  = 0x0100,  // Array size determined at runtime (REDIM)
    STATIC_ARRAY   = 0x0200,  // Array size fixed at compile time
};

// Helper to combine flags
inline TypeAttribute operator|(TypeAttribute a, TypeAttribute b) {
    return static_cast<TypeAttribute>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
```

### 2.3 Complete Type Descriptor

```cpp
struct TypeDescriptor {
    BaseType baseType;
    uint32_t attributes;        // Bitfield of TypeAttribute flags
    uint32_t udtTypeId;         // For USER_DEFINED: unique type ID (0 = none)
    std::vector<int> arrayDims; // For arrays: dimension sizes (-1 = dynamic/REDIM)
    
    // Constructors
    TypeDescriptor() 
        : baseType(BaseType::UNKNOWN), attributes(0), udtTypeId(0) {}
    
    explicit TypeDescriptor(BaseType base) 
        : baseType(base), attributes(0), udtTypeId(0) {}
    
    TypeDescriptor(BaseType base, uint32_t attrs) 
        : baseType(base), attributes(attrs), udtTypeId(0) {}
    
    TypeDescriptor(uint32_t udtId)  // UDT constructor
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
    
    bool isHidden() const {
        return (attributes & (uint32_t)TypeAttribute::IS_HIDDEN) != 0;
    }
    
    bool isUnsigned() const {
        return (attributes & (uint32_t)TypeAttribute::IS_UNSIGNED) != 0 ||
               baseType == BaseType::UBYTE ||
               baseType == BaseType::USHORT ||
               baseType == BaseType::UINTEGER ||
               baseType == BaseType::ULONG;
    }
    
    bool isUserDefined() const { 
        return baseType == BaseType::USER_DEFINED; 
    }
    
    bool isInteger() const {
        return baseType == BaseType::BYTE || baseType == BaseType::UBYTE ||
               baseType == BaseType::SHORT || baseType == BaseType::USHORT ||
               baseType == BaseType::INTEGER || baseType == BaseType::UINTEGER ||
               baseType == BaseType::LONG || baseType == BaseType::ULONG;
    }
    
    bool isFloat() const {
        return baseType == BaseType::SINGLE || baseType == BaseType::DOUBLE;
    }
    
    bool isNumeric() const {
        return isInteger() || isFloat();
    }
    
    bool isString() const {
        return baseType == BaseType::STRING || baseType == BaseType::UNICODE;
    }
    
    // Get size in bytes
    size_t sizeInBytes() const {
        switch (baseType) {
            case BaseType::BYTE:
            case BaseType::UBYTE:     return 1;
            case BaseType::SHORT:
            case BaseType::USHORT:    return 2;
            case BaseType::INTEGER:
            case BaseType::UINTEGER:
            case BaseType::SINGLE:    return 4;
            case BaseType::LONG:
            case BaseType::ULONG:
            case BaseType::DOUBLE:
            case BaseType::PTR:       return 8;
            case BaseType::STRING:
            case BaseType::UNICODE:   return 16;  // String descriptor size
            case BaseType::ARRAY_DESC: return 32; // Array descriptor size
            case BaseType::STRING_DESC: return 16; // String descriptor size
            default: return 0;
        }
    }
    
    // Get integer rank for promotion (higher = wider)
    int integerRank() const {
        switch (baseType) {
            case BaseType::BYTE:
            case BaseType::UBYTE:     return 1;
            case BaseType::SHORT:
            case BaseType::USHORT:    return 2;
            case BaseType::INTEGER:
            case BaseType::UINTEGER:  return 3;
            case BaseType::LONG:
            case BaseType::ULONG:     return 4;
            default: return 0;
        }
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
    
    // Convert to QBE type character
    char toQBEType() const {
        if (isArray() || isPointer() || isString()) {
            return 'l';  // Arrays, pointers, strings are all pointers
        }
        
        switch (baseType) {
            case BaseType::BYTE:
            case BaseType::UBYTE:
            case BaseType::SHORT:
            case BaseType::USHORT:
            case BaseType::INTEGER:
            case BaseType::UINTEGER:  return 'w';  // 32-bit integer
            
            case BaseType::LONG:
            case BaseType::ULONG:
            case BaseType::PTR:
            case BaseType::ARRAY_DESC:
            case BaseType::STRING_DESC:
            case BaseType::LOOP_INDEX: return 'l';  // 64-bit integer/pointer
            
            case BaseType::SINGLE:    return 's';  // 32-bit float
            case BaseType::DOUBLE:    return 'd';  // 64-bit float
            
            case BaseType::USER_DEFINED: return 'l';  // UDT is a pointer
            
            default: return 'w';  // Default to word
        }
    }
    
    // Convert to QBE extended type (for memory operations)
    std::string toQBEExtendedType() const {
        switch (baseType) {
            case BaseType::BYTE:      return "sb";  // signed byte
            case BaseType::UBYTE:     return "ub";  // unsigned byte
            case BaseType::SHORT:     return "sh";  // signed half
            case BaseType::USHORT:    return "uh";  // unsigned half
            case BaseType::INTEGER:
            case BaseType::UINTEGER:  return "w";   // word
            case BaseType::LONG:
            case BaseType::ULONG:     return "l";   // long
            case BaseType::SINGLE:    return "s";   // single
            case BaseType::DOUBLE:    return "d";   // double
            default: return "l";
        }
    }
    
    // String representation for error messages
    std::string toString() const;
    
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
};
```

---

## 3. Type Suffixes

### 3.1 Traditional BASIC Suffixes (Enhanced)

```cpp
// Map type suffix to BaseType
BaseType suffixToBaseType(char suffix) {
    switch (suffix) {
        case '%': return BaseType::INTEGER;   // Traditional integer (32-bit)
        case '&': return BaseType::LONG;      // Traditional long (64-bit)
        case '!': return BaseType::SINGLE;    // Traditional single
        case '#': return BaseType::DOUBLE;    // Traditional double
        case '$': return BaseType::STRING;    // String (ASCII or Unicode based on mode)
        case '@': return BaseType::BYTE;      // NEW: Byte suffix
        case '^': return BaseType::SHORT;     // NEW: Short suffix
        default:  return BaseType::UNKNOWN;
    }
}

// Map suffix character back to string for error messages
const char* suffixName(char suffix) {
    switch (suffix) {
        case '%': return "INTEGER";
        case '&': return "LONG";
        case '!': return "SINGLE";
        case '#': return "DOUBLE";
        case '$': return "STRING";
        case '@': return "BYTE";
        case '^': return "SHORT";
        default:  return "UNKNOWN";
    }
}
```

### 3.2 Examples

```basic
' Integer types
DIM b@ AS BYTE        ' or: b@ (8-bit)
DIM s^ AS SHORT       ' or: s^ (16-bit)
DIM i% AS INTEGER     ' or: i% (32-bit) - traditional
DIM n& AS LONG        ' or: n& (64-bit) - traditional

' Float types
DIM x! AS SINGLE      ' or: x! (32-bit float) - traditional
DIM y# AS DOUBLE      ' or: y# (64-bit float) - traditional

' String types
DIM name$ AS STRING   ' or: name$ (byte or UTF-32 based on mode)

' Arrays with suffixes
DIM bytes@(100)       ' Array of BYTEs
DIM counts&(1000)     ' Array of LONGs
```

---

## 4. Hidden/Internal Types

These types are not directly accessible to user code but are used internally by the compiler and runtime.

### 4.1 Loop Index Type

**Purpose**: All loop variables (FOR/NEXT, FOR IN, etc.) are internally promoted to LONG (64-bit) for consistency and to avoid overflow issues.

```cpp
// Internal representation of loop variables
struct ForLoopContext {
    std::string variable;      // User's variable name
    TypeDescriptor userType;   // User's declared type (e.g., INTEGER)
    TypeDescriptor indexType;  // Always LONG internally
    
    ForLoopContext() : indexType(BaseType::LOOP_INDEX) {
        indexType.attributes |= (uint32_t)TypeAttribute::IS_HIDDEN;
    }
};
```

**Example**:

```basic
DIM i% AS INTEGER
FOR i% = 1 TO 1000000
    ' i% is INTEGER to user, but loop counter is internally LONG
    ' On assignment back to i%, value is truncated to 32-bit
NEXT i%
```

**Rationale**:
- Avoids overflow in long-running loops
- Consistent behavior across all loop types
- Simplifies codegen (all loops use same index type)

### 4.2 Array Descriptor Type

**Purpose**: Internal structure holding array metadata (dimensions, bounds, element size, data pointer).

```cpp
// Internal array descriptor structure
struct ArrayDescriptorLayout {
    int64_t magic;           // Magic number for validation
    int32_t elementSize;     // Size of each element in bytes
    int32_t rank;            // Number of dimensions
    int64_t totalElements;   // Total number of elements
    void* data;              // Pointer to actual data
    int64_t dims[8];         // Dimension sizes (max 8 dimensions)
    int64_t lbounds[8];      // Lower bounds (OPTION BASE 0 or 1)
};

// Type descriptor for array descriptors
TypeDescriptor makeArrayDescriptorType() {
    TypeDescriptor desc(BaseType::ARRAY_DESC);
    desc.attributes |= (uint32_t)TypeAttribute::IS_HIDDEN;
    return desc;
}
```

**Usage**:
- Arrays are represented as pointers to descriptors: `l` in QBE
- Element access requires descriptor lookup for bounds checking
- REDIM modifies the descriptor

### 4.3 String Descriptor Type

**Purpose**: Internal structure holding string metadata (length, capacity, codepoint count, data pointer).

```cpp
// Internal string descriptor structure
struct StringDescriptorLayout {
    int64_t magic;           // Magic number for validation
    int32_t encoding;        // 0 = ASCII/byte, 1 = UTF-32
    int32_t capacity;        // Allocated capacity in bytes
    int64_t byteLength;      // Length in bytes
    int64_t codePointCount;  // Length in codepoints (for Unicode)
    void* data;              // Pointer to string data
};

// Type descriptor for string descriptors
TypeDescriptor makeStringDescriptorType() {
    TypeDescriptor desc(BaseType::STRING_DESC);
    desc.attributes |= (uint32_t)TypeAttribute::IS_HIDDEN;
    return desc;
}
```

**Usage**:
- All strings (STRING and UNICODE) use descriptors
- Allows efficient substring operations
- Reference counted for memory management

### 4.4 Array Index Types

**Purpose**: Track whether array uses 32-bit or 64-bit indices based on size.

```cpp
// Determine index type based on array size
TypeDescriptor getArrayIndexType(const TypeDescriptor& arrayType) {
    if (!arrayType.isArray()) {
        return TypeDescriptor(BaseType::UNKNOWN);
    }
    
    // Calculate total elements
    int64_t totalElements = 1;
    for (int dim : arrayType.arrayDims) {
        if (dim == -1) {
            // Dynamic array, assume large
            return TypeDescriptor(BaseType::LONG);
        }
        totalElements *= dim;
    }
    
    // If array has more than 2^31 elements, use LONG indices
    if (totalElements > 0x7FFFFFFF) {
        return TypeDescriptor(BaseType::LONG);
    }
    
    // Otherwise, INTEGER indices are sufficient
    return TypeDescriptor(BaseType::INTEGER);
}
```

**Example**:

```basic
' Small array: INTEGER indices
DIM small%(100, 100)           ' 10,000 elements -> INTEGER index

' Huge array: LONG indices
DIM huge#(10000, 10000)        ' 100,000,000 elements -> LONG index

' String indices
DIM s$ = "Hello"
x% = ASC(MID$(s$, 2))          ' Index is INTEGER (position < 2^31)

DIM unicode$ = "Unicode"
c& = CODE(MID$(unicode$, 2))   ' Unicode codepoint access
```

---

## 5. Type Coercion Rules

### 5.1 Implicit Conversions (Safe)

These conversions happen automatically without warning:

```
BYTE    -> SHORT -> INTEGER -> LONG
UBYTE   -> USHORT -> UINTEGER -> ULONG
INTEGER -> SINGLE -> DOUBLE
LONG    -> DOUBLE
STRING  <-> UNICODE (in mixed mode)
```

**Rationale**: Widening conversions don't lose precision (mostly - see caveats).

**Caveat**: `LONG -> DOUBLE` can lose precision for integers > 2^53.

### 5.2 Explicit Conversions (Require Cast or Built-in Function)

These conversions require explicit action:

```
INTEGER -> BYTE     ' May overflow: use CBYTE()
LONG -> INTEGER     ' May overflow: use CINT()
DOUBLE -> SINGLE    ' May lose precision: use CSNG()
DOUBLE -> LONG      ' Truncates fractional part: use CLNG() or INT()
SINGLE -> INTEGER   ' Truncates fractional part: use CINT()
Unsigned <-> Signed ' May change sign: use casting
```

### 5.3 Coercion Rules Matrix

| From → To | BYTE | SHORT | INTEGER | LONG | SINGLE | DOUBLE | STRING |
|-----------|------|-------|---------|------|--------|--------|--------|
| BYTE      | =    | Auto  | Auto    | Auto | Auto   | Auto   | STR$() |
| SHORT     | Warn | =     | Auto    | Auto | Auto   | Auto   | STR$() |
| INTEGER   | Warn | Warn  | =       | Auto | Auto   | Auto   | STR$() |
| LONG      | Warn | Warn  | Warn    | =    | Warn   | Auto   | STR$() |
| SINGLE    | Func | Func  | Func    | Func | =      | Auto   | STR$() |
| DOUBLE    | Func | Func  | Func    | Func | Warn   | =      | STR$() |
| STRING    | VAL  | VAL   | VAL     | VAL  | VAL    | VAL    | =      |

**Legend**:
- `=`: Same type, no conversion
- `Auto`: Automatic (implicit) conversion
- `Warn`: Automatic but with warning (lossy)
- `Func`: Requires explicit function call
- `VAL`, `STR$()`: String conversion functions

### 5.4 Implementation

```cpp
enum class CoercionKind {
    NONE,           // No conversion needed
    IMPLICIT,       // Safe implicit conversion
    IMPLICIT_LOSSY, // Lossy implicit conversion (warn)
    EXPLICIT,       // Requires explicit cast/function
    FORBIDDEN       // Cannot convert
};

struct CoercionResult {
    CoercionKind kind;
    std::string functionName;  // For EXPLICIT: required function name
    std::string warning;       // For IMPLICIT_LOSSY: warning message
};

CoercionResult checkCoercion(const TypeDescriptor& from, const TypeDescriptor& to) {
    // Same type
    if (from == to) {
        return {CoercionKind::NONE, "", ""};
    }
    
    // Array vs scalar mismatch
    if (from.isArray() != to.isArray()) {
        return {CoercionKind::FORBIDDEN, "", "Cannot convert between array and scalar"};
    }
    
    // UDT mismatch
    if (from.isUserDefined() || to.isUserDefined()) {
        if (from.udtTypeId != to.udtTypeId) {
            return {CoercionKind::FORBIDDEN, "", "Cannot convert between different user-defined types"};
        }
        return {CoercionKind::NONE, "", ""};
    }
    
    // String <-> numeric
    if (from.isString() != to.isString()) {
        if (from.isString()) {
            return {CoercionKind::EXPLICIT, "VAL", "Use VAL() to convert string to number"};
        } else {
            return {CoercionKind::EXPLICIT, "STR$", "Use STR$() to convert number to string"};
        }
    }
    
    // String <-> Unicode (allowed in mixed mode)
    if (from.isString() && to.isString()) {
        return {CoercionKind::IMPLICIT, "", ""};
    }
    
    // Numeric conversions
    if (from.isNumeric() && to.isNumeric()) {
        return checkNumericCoercion(from, to);
    }
    
    return {CoercionKind::FORBIDDEN, "", "Incompatible types"};
}

CoercionResult checkNumericCoercion(const TypeDescriptor& from, const TypeDescriptor& to) {
    // Integer -> Integer
    if (from.isInteger() && to.isInteger()) {
        int fromRank = from.integerRank();
        int toRank = to.integerRank();
        
        if (fromRank <= toRank) {
            // Widening: safe
            return {CoercionKind::IMPLICIT, "", ""};
        } else {
            // Narrowing: lossy
            std::string funcName;
            switch (to.baseType) {
                case BaseType::BYTE:    funcName = "CBYTE"; break;
                case BaseType::SHORT:   funcName = "CSHORT"; break;
                case BaseType::INTEGER: funcName = "CINT"; break;
                case BaseType::LONG:    funcName = "CLNG"; break;
                default: funcName = "cast";
            }
            return {
                CoercionKind::IMPLICIT_LOSSY,
                "",
                "Narrowing conversion from " + from.toString() + " to " + to.toString() + 
                " may lose data. Consider using " + funcName + "()"
            };
        }
    }
    
    // Integer -> Float
    if (from.isInteger() && to.isFloat()) {
        if (from.baseType == BaseType::LONG && to.baseType == BaseType::SINGLE) {
            // LONG -> SINGLE may lose precision
            return {
                CoercionKind::IMPLICIT_LOSSY,
                "",
                "Converting LONG to SINGLE may lose precision for large values"
            };
        }
        return {CoercionKind::IMPLICIT, "", ""};
    }
    
    // Float -> Integer
    if (from.isFloat() && to.isInteger()) {
        std::string funcName = (to.baseType == BaseType::LONG) ? "CLNG" : "CINT";
        return {
            CoercionKind::EXPLICIT,
            funcName,
            "Use " + funcName + "() or INT() to convert float to integer"
        };
    }
    
    // Float -> Float
    if (from.isFloat() && to.isFloat()) {
        if (from.baseType == BaseType::DOUBLE && to.baseType == BaseType::SINGLE) {
            // DOUBLE -> SINGLE loses precision
            return {
                CoercionKind::IMPLICIT_LOSSY,
                "",
                "Converting DOUBLE to SINGLE may lose precision. Consider using CSNG()"
            };
        }
        return {CoercionKind::IMPLICIT, "", ""};
    }
    
    return {CoercionKind::FORBIDDEN, "", "Invalid numeric conversion"};
}
```

### 5.5 Assignment Coercion

```cpp
void SemanticAnalyzer::validateAssignment(const TypeDescriptor& targetType,
                                          const TypeDescriptor& valueType,
                                          const SourceLocation& loc) {
    auto coercion = checkCoercion(valueType, targetType);
    
    switch (coercion.kind) {
        case CoercionKind::NONE:
        case CoercionKind::IMPLICIT:
            // OK
            break;
            
        case CoercionKind::IMPLICIT_LOSSY:
            warning(coercion.warning, loc);
            break;
            
        case CoercionKind::EXPLICIT:
            error(SemanticErrorType::TYPE_MISMATCH,
                  "Cannot implicitly convert " + valueType.toString() + 
                  " to " + targetType.toString() + ". " + coercion.warning,
                  loc);
            break;
            
        case CoercionKind::FORBIDDEN:
            error(SemanticErrorType::TYPE_MISMATCH,
                  "Cannot convert " + valueType.toString() + 
                  " to " + targetType.toString() + ": " + coercion.warning,
                  loc);
            break;
    }
}
```

---

## 6. Type Inference Enhancements

### 6.1 Enhanced Type Inference

```cpp
class SemanticAnalyzer {
    // Main type inference (now returns TypeDescriptor, not VariableType)
    TypeDescriptor inferExpressionType(const Expression& expr);
    
    // Specific inference methods
    TypeDescriptor inferLiteralType(const Expression& expr);
    TypeDescriptor inferVariableType(const VariableExpression& expr);
    TypeDescriptor inferArrayAccessType(const ArrayAccessExpression& expr);
    TypeDescriptor inferFunctionCallType(const FunctionCallExpression& expr);
    TypeDescriptor inferBinaryExpressionType(const BinaryExpression& expr);
    TypeDescriptor inferUnaryExpressionType(const UnaryExpression& expr);
    TypeDescriptor inferMemberAccessType(const MemberAccessExpression& expr);
    
    // Type promotion for binary operators
    TypeDescriptor promoteTypes(const TypeDescriptor& left, const TypeDescriptor& right);
};
```

### 6.2 Literal Type Inference

```cpp
TypeDescriptor SemanticAnalyzer::inferLiteralType(const Expression& expr) {
    if (expr.getType() == ASTNodeType::EXPR_NUMBER) {
        const auto& numExpr = static_cast<const NumberExpression&>(expr);
        
        // Check for explicit suffix
        if (numExpr.hasSuffix) {
            return TypeDescriptor(suffixToBaseType(numExpr.suffix));
        }
        
        // Infer from value
        if (numExpr.isInteger) {
            int64_t val = numExpr.intValue;
            
            if (val >= -128 && val <= 127) {
                return TypeDescriptor(BaseType::BYTE);
            } else if (val >= -32768 && val <= 32767) {
                return TypeDescriptor(BaseType::SHORT);
            } else if (val >= -2147483648LL && val <= 2147483647LL) {
                return TypeDescriptor(BaseType::INTEGER);
            } else {
                return TypeDescriptor(BaseType::LONG);
            }
        } else {
            // Floating point: default to SINGLE unless too large
            if (numExpr.floatValue >= -3.4e38 && numExpr.floatValue <= 3.4e38) {
                return TypeDescriptor(BaseType::SINGLE);
            } else {
                return TypeDescriptor(BaseType::DOUBLE);
            }
        }
    }
    
    if (expr.getType() == ASTNodeType::EXPR_STRING) {
        if (m_symbolTable.unicodeMode) {
            return TypeDescriptor(BaseType::UNICODE);
        } else {
            return TypeDescriptor(BaseType::STRING);
        }
    }
    
    return TypeDescriptor(BaseType::UNKNOWN);
}
```

### 6.3 Binary Operator Type Promotion

```cpp
TypeDescriptor SemanticAnalyzer::promoteTypes(const TypeDescriptor& left, 
                                              const TypeDescriptor& right) {
    // String concatenation
    if (left.isString() || right.isString()) {
        // If either is UNICODE, result is UNICODE
        if (left.baseType == BaseType::UNICODE || right.baseType == BaseType::UNICODE) {
            return TypeDescriptor(BaseType::UNICODE);
        }
        return TypeDescriptor(BaseType::STRING);
    }
    
    // Numeric promotion hierarchy: BYTE < SHORT < INTEGER < LONG < SINGLE < DOUBLE
    if (left.isFloat() || right.isFloat()) {
        // Float promotion
        if (left.baseType == BaseType::DOUBLE || right.baseType == BaseType::DOUBLE) {
            return TypeDescriptor(BaseType::DOUBLE);
        }
        return TypeDescriptor(BaseType::SINGLE);
    }
    
    // Integer promotion
    if (left.isInteger() && right.isInteger()) {
        int leftRank = left.integerRank();
        int rightRank = right.integerRank();
        
        // Promote to wider type
        if (leftRank > rightRank) {
            return left;
        } else if (rightRank > leftRank) {
            return right;
        } else {
            // Same rank: preserve signedness
            if (left.isUnsigned() && right.isUnsigned()) {
                return left;  // Both unsigned
            } else {
                // At least one signed, return signed variant
                switch (left.baseType) {
                    case BaseType::UBYTE: return TypeDescriptor(BaseType::BYTE);
                    case BaseType::USHORT: return TypeDescriptor(BaseType::SHORT);
                    case BaseType::UINTEGER: return TypeDescriptor(BaseType::INTEGER);
                    case BaseType::ULONG: return TypeDescriptor(BaseType::LONG);
                    default: return left;
                }
            }
        }
    }
    
    // Default to SINGLE
    return TypeDescriptor(BaseType::SINGLE);
}
```

---

## 7. Code Generation Mapping

### 7.1 QBE Type Emission

```cpp
std::string TypeDescriptor::toQBETypeString() const {
    if (isArray() || baseType == BaseType::PTR) {
        return "l";  // Pointers are always 'l' (64-bit)
    }
    
    switch (baseType) {
        // Extended types for memory operations
        case BaseType::BYTE:      return "sb";  // signed byte (load/store)
        case BaseType::UBYTE:     return "ub";  // unsigned byte
        case BaseType::SHORT:     return "sh";  // signed half
        case BaseType::USHORT:    return "uh";  // unsigned half
        
        // Base types for computations
        case BaseType::INTEGER:
        case BaseType::UINTEGER:  return "w";   // 32-bit word
        
        case BaseType::LONG:
        case BaseType::ULONG:
        case BaseType::LOOP_INDEX: return "l";  // 64-bit long
        
        case BaseType::SINGLE:    return "s";   // 32-bit float
        case BaseType::DOUBLE:    return "d";   // 64-bit float
        
        case BaseType::STRING:
        case BaseType::UNICODE:
        case BaseType::STRING_DESC:
        case BaseType::ARRAY_DESC:
        case BaseType::USER_DEFINED: return "l";  // All descriptors/UDTs are pointers
        
        default: return "w";  // Default to word
    }
}

// For variable declarations
std::string emitQBETypeForVar(const TypeDescriptor& type) {
    // Variables in QBE use base types (w, l, s, d)
    // Extended types (b, h) are only used in memory operations
    char baseType = type.toQBEType();
    return std::string(1, baseType);
}

// For memory operations (load/store)
std::string emitQBELoadStore(const TypeDescriptor& type, bool isLoad) {
    std::string instr = isLoad ? "load" : "store";
    std::string extType = type.toQBEExtendedType();
    return instr + extType;
}
```

### 7.2 Example Codegen

```basic
DIM b@ AS BYTE
DIM s^ AS SHORT
DIM i% AS INTEGER
DIM n& AS LONG
DIM x! AS SINGLE
DIM y# AS DOUBLE

b@ = 100
s^ = 30000
i% = 1000000
n& = 9000000000
x! = 3.14
y# = 2.71828
```

Generated QBE:

```qbe
# Variables
%b =l alloc4 1      # BYTE: 1 byte allocation
%s =l alloc4 2      # SHORT: 2 byte allocation
%i =l alloc4 4      # INTEGER: 4 byte allocation
%n =l alloc8 8      # LONG: 8 byte allocation
%x =l alloc4 4      # SINGLE: 4 byte allocation
%y =l alloc8 8      # DOUBLE: 8 byte allocation

# Assignments
storesb 100, %b     # Store signed byte
storesh 30000, %s   # Store signed half
storew 1000000, %i  # Store word
storel 9000000000, %n  # Store long
stores s_3.14, %x   # Store single
stored d_2.71828, %y   # Store double

# Load with extension
%b_val =w loadsb %b    # Load byte, sign-extend to word
%s_val =w loadsh %s    # Load half, sign-extend to word
%i_val =w loadw %i     # Load word
%n_val =l loadl %n     # Load long
%x_val =s loads %x     # Load single
%y_val =d loadd %y     # Load double
```

---

## 8. Examples

### 8.1 Type Declarations

```basic
' All integer types
DIM tiny@ AS BYTE          ' -128 to 127
DIM small^ AS SHORT        ' -32768 to 32767
DIM medium% AS INTEGER     ' -2^31 to 2^31-1
DIM big& AS LONG           ' -2^63 to 2^63-1

' Unsigned variants (explicit keyword)
DIM utiny AS UBYTE         ' 0 to 255
DIM usmall AS USHORT       ' 0 to 65535
DIM umedium AS UINTEGER    ' 0 to 2^32-1
DIM ubig AS ULONG          ' 0 to 2^64-1

' Float types
DIM x! AS SINGLE           ' 32-bit float
DIM y# AS DOUBLE           ' 64-bit float

' Arrays
DIM bytes@(256) AS BYTE    ' Array of bytes
DIM indices&(10000) AS LONG  ' Array of longs (for large arrays)
```

### 8.2 Type Coercion Examples

```basic
' Safe implicit conversions (no warning)
DIM b@ AS BYTE
DIM i% AS INTEGER
DIM n& AS LONG
DIM x! AS SINGLE
DIM y# AS DOUBLE

b@ = 10
i% = b@        ' BYTE -> INTEGER: OK
n& = i%        ' INTEGER -> LONG: OK
x! = i%        ' INTEGER -> SINGLE: OK
y# = x!        ' SINGLE -> DOUBLE: OK

' Lossy conversions (warning)
i% = n&        ' WARNING: LONG -> INTEGER may overflow
x! = n&        ' WARNING: LONG -> SINGLE may lose precision
b@ = i%        ' WARNING: INTEGER -> BYTE may overflow

' Explicit conversions required (error without cast)
i% = x!        ' ERROR: use CINT(x!) or INT(x!)
n& = y#        ' ERROR: use CLNG(y#)
b@ = i%        ' ERROR: use CBYTE(i%)

' String conversions
DIM s$ AS STRING
s$ = "123"
i% = VAL(s$)   ' Explicit: string to number
s$ = STR$(i%)  ' Explicit: number to string
```

### 8.3 Loop Index Promotion

```basic
' User sees INTEGER, but loop internally uses LONG
DIM i% AS INTEGER

FOR i% = 1 TO 10000000
    ' Loop counter is LONG internally to avoid overflow
    PRINT i%  ' Converted back to INTEGER on access
NEXT i%

' Explicit LONG loop
DIM bigIdx& AS LONG
FOR bigIdx& = 1 TO 9999999999&
    ' Loop uses LONG throughout
NEXT bigIdx&
```

### 8.4 Array Index Types

```basic
' Small array: INTEGER indices internally
DIM small%(100, 100)
small%(50, 50) = 42  ' Index calculation uses INTEGER

' Huge array: LONG indices internally
DIM huge#(100000, 100000)  ' 10 billion elements!
huge#(50000, 50000) = 3.14  ' Index calculation uses LONG
```

### 8.5 User-Defined Types with Varied Field Types

```basic
TYPE Pixel
    r@ AS BYTE       ' Red: 0-255
    g@ AS BYTE       ' Green: 0-255
    b@ AS BYTE       ' Blue: 0-255
    a@ AS BYTE       ' Alpha: 0-255
END TYPE

TYPE GameObject
    id& AS LONG      ' Unique ID (64-bit for many objects)
    x# AS DOUBLE     ' Position X
    y# AS DOUBLE     ' Position Y
    health% AS INTEGER  ' Health points
    name$ AS STRING  ' Object name
END TYPE

DIM pixel AS Pixel
pixel.r@ = 255
pixel.g@ = 128
pixel.b@ = 64
pixel.a@ = 255

DIM player AS GameObject
player.id& = 1000000000&
player.x# = 100.5
player.y# = 200.75
player.health% = 100
player.name$ = "Player1"
```

---

## 9. Migration Plan

### Phase 1: Add New Type System (Week 1)
- [ ] Add `BaseType` enum with all types
- [ ] Add `TypeAttribute` flags
- [ ] Add `TypeDescriptor` struct
- [ ] Add type registry to `SymbolTable`
- [ ] Keep old `VariableType` for compatibility
- [ ] Add conversion functions

### Phase 2: Update Type Inference (Week 2)
- [ ] Change all `infer*Type()` methods to return `TypeDescriptor`
- [ ] Update `promoteTypes()` for new system
- [ ] Implement coercion checking
- [ ] Add warning generation for lossy conversions

### Phase 3: Update Symbols (Week 3)
- [ ] Migrate `TypeSymbol::Field` to `TypeDescriptor`
- [ ] Migrate `FunctionSymbol` parameter types
- [ ] Migrate `VariableSymbol` type field
- [ ] Migrate `ArraySymbol` type field

### Phase 4: Update Parser & Semantic Analysis (Week 4)
- [ ] Add new suffix parsing (`@` for BYTE, `^` for SHORT)
- [ ] Update `processDimStatement()` for new types
- [ ] Update `processTypeDeclarationStatement()` for field types
- [ ] Update type checking in assignments
- [ ] Update type checking in function calls

### Phase 5: Update Code Generation (Week 5)
- [ ] Update QBE type emission
- [ ] Handle extended types in loads/stores
- [ ] Update array descriptor generation
- [ ] Update string descriptor generation
- [ ] Test with all type combinations

### Phase 6: Testing & Documentation (Week 6)
- [ ] Add comprehensive type system tests
- [ ] Test all coercion rules
- [ ] Test hidden type generation
- [ ] Update user documentation
- [ ] Remove old `VariableType` enum

---

## 10. Benefits

### 10.1 Precision
- **Exact QBE alignment**: Types map 1:1 to QBE's type system
- **No ambiguity**: Each type has clear semantics and QBE representation
- **Better optimization**: QBE can optimize knowing exact types

### 10.2 Safety
- **Catch errors early**: Type mismatches caught in semantic analysis, not QBE
- **Lossy conversion warnings**: User notified of potential data loss
- **Array bounds**: Proper index types prevent overflow

### 10.3 Performance
- **Optimal code**: Use smallest type sufficient (BYTE vs LONG)
- **Cache efficiency**: Smaller types = better cache utilization
- **SIMD potential**: Aligned types enable vectorization

### 10.4 Expressiveness
- **Full integer range**: BYTE, SHORT, INTEGER, LONG cover all needs
- **Unsigned support**: For bit manipulation and large positive values
- **Hidden types**: Internal optimization without user confusion

### 10.5 Compatibility
- **Traditional BASIC**: Suffixes (%, &, !, #, $) still work
- **Modern explicit types**: AS INTEGER, AS LONG, etc.
- **Gradual adoption**: Can mix old and new styles

---

## 11. Open Questions

### Q1: Should unsigned types be user-accessible?
**Proposal**: Yes, but only via explicit `AS UBYTE`, not via suffix.

**Rationale**: 
- Useful for bit manipulation
- Avoids confusion with existing suffixes
- Clear intent in code

### Q2: How to handle BYTE overflow in loops?
**Proposal**: All loop indices are internally LONG, even if user declares as BYTE.

**Example**:
```basic
DIM b@ AS BYTE
FOR b@ = 0 TO 300  ' Would overflow BYTE (max 255)
    ' Loop works because internal counter is LONG
    ' b@ = internal_counter MOD 256 (wrapped)
NEXT b@
' WARNING: Loop wrapped, b@ ends at 300 MOD 256 = 44
```

### Q3: String vs Unicode automatic conversion?
**Proposal**: Allow in `OPTION MIXED` mode, error in strict mode.

---

## 12. Summary

This type system provides:

1. **Complete QBE alignment**: All QBE types (w, l, s, d, sb, ub, sh, uh) represented
2. **Traditional BASIC compatibility**: Suffixes (%, &, !, #, $, @, ^) supported
3. **Hidden types**: Loop indices, array/string descriptors handled internally
4. **Smart coercion**: Clear rules with warnings for lossy conversions
5. **Extensibility**: Easy to add future types (complex numbers, decimals, etc.)

**Result**: Type-safe compilation with QBE, clear error messages, and optimal code generation.