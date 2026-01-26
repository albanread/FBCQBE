# Semantic Analyzer Review: Current Weaknesses and Issues

## Executive Summary

This document reviews the current FasterBASIC semantic analyzer implementation, highlighting specific weaknesses that QBE's type checking exposes. The analyzer uses a simple enum-based type system that cannot adequately represent complex types like arrays, user-defined types, or combinations thereof.

---

## Critical Weaknesses

### 1. Type System Cannot Represent Array Types

**Location**: `fasterbasic_semantic.h:32-41`

```cpp
enum class VariableType {
    INT,        // Integer (%)
    FLOAT,      // Single precision (! or default)
    DOUBLE,     // Double precision (#)
    STRING,     // String ($) - byte-based
    UNICODE,    // Unicode string ($) - codepoint array (OPTION UNICODE mode)
    VOID,       // No return value (for SUB)
    USER_DEFINED, // User-defined type (TYPE...END TYPE)
    UNKNOWN     // Not yet determined
};
```

**Problem**: 
- No way to express "array of INTEGER" vs "INTEGER scalar"
- `VariableSymbol` and `ArraySymbol` are completely separate types
- Type inference returns `VariableType::INT` for both `x%` and `arr%(10)`

**Impact**:
- Cannot perform accurate type checking in assignments
- QBE receives incorrect type information (treating array as scalar or vice versa)
- Member access on array elements of UDT cannot be properly typed

**Example of Failure**:

```basic
DIM x AS INTEGER
DIM arr(10) AS INTEGER

x = arr    ' Should error: cannot assign array to scalar
           ' Currently: no error, or wrong error
```

---

### 2. USER_DEFINED Type Is Not Specific

**Location**: `fasterbasic_semantic.h:39`, `fasterbasic_semantic.h:65`

```cpp
struct VariableSymbol {
    std::string name;
    VariableType type;        // Just says USER_DEFINED
    std::string typeName;     // Separate string field
    // ...
};
```

**Problem**:
- `VariableType::USER_DEFINED` doesn't identify WHICH user-defined type
- Type identity relies on string comparison of `typeName`
- No type ID system for fast, reliable type matching
- String typos can cause silent bugs

**Impact**:
- Weak type checking between different UDTs
- Cannot easily check if two UDTs are the same type
- Error messages are vague ("type mismatch" instead of "cannot assign Sprite to Point")

**Example of Current Code**:

```cpp
// fasterbasic_semantic.cpp:2408-2418
for (size_t i = 0; i < expr.arguments.size() && i < sym->parameterTypes.size(); ++i) {
    VariableType argType = inferExpressionType(*expr.arguments[i]);
    VariableType paramType = sym->parameterTypes[i];
    std::string paramTypeName = i < sym->parameterTypeNames.size() ? 
                                sym->parameterTypeNames[i] : "";
    
    // For user-defined types, check type compatibility
    // ... but paramType is just USER_DEFINED, no way to check which UDT!
}
```

---

### 3. Weak Type Compatibility Checking

**Location**: `fasterbasic_semantic.cpp:2527-2548`

```cpp
void SemanticAnalyzer::checkTypeCompatibility(VariableType expected, VariableType actual,
                                              const SourceLocation& loc, const std::string& context) {
    if (expected == VariableType::UNKNOWN || actual == VariableType::UNKNOWN) {
        return;  // Can't check - TOO LENIENT!
    }
    
    // String to numeric or vice versa is an error
    bool expectedString = (expected == VariableType::STRING || expected == VariableType::UNICODE);
    bool actualString = (actual == VariableType::STRING || actual == VariableType::UNICODE);
    
    if (expectedString != actualString) {
        error(SemanticErrorType::TYPE_MISMATCH,
              "Type mismatch in " + context + ": cannot assign " +
              std::string(typeToString(actual)) + " to " + std::string(typeToString(expected)),
              loc);
    }
    // That's it! No checking of:
    // - Array vs scalar
    // - Specific UDT types
    // - Numeric precision compatibility
}
```

**Problems**:
1. Returns early if either type is UNKNOWN (silent pass)
2. Only checks string vs numeric category
3. No array dimension checking
4. No UDT type ID checking
5. No validation of numeric conversions (INT to DOUBLE ok, but DOUBLE to INT?)

**Impact**:
- Many type errors slip through
- QBE catches them later with cryptic errors
- Poor error messages for users

---

### 4. Incomplete Member Access Type Inference

**Location**: `fasterbasic_semantic.cpp:2200-2235`

```cpp
VariableType SemanticAnalyzer::inferMemberAccessType(const MemberAccessExpression& expr) {
    // Infer the type of a member access expression (e.g., point.X)
    
    // First, determine the type of the base object
    VariableType baseType = VariableType::UNKNOWN;
    std::string baseTypeName;
    
    // Check if the object is a variable
    if (expr.object->getType() == ASTNodeType::EXPR_VARIABLE) {
        const VariableExpression* varExpr = static_cast<const VariableExpression*>(expr.object.get());
        VariableSymbol* varSym = lookupVariable(varExpr->name);
        if (varSym) {
            baseType = varSym->type;
            // For user-defined types, we need to find the type name
            // Variables of user-defined types store the type name (we'll enhance this later)
        }
    } else if (expr.object->getType() == ASTNodeType::EXPR_ARRAY_ACCESS) {
        // Array element access
        const ArrayAccessExpression* arrayExpr = static_cast<const ArrayAccessExpression*>(expr.object.get());
        ArraySymbol* arraySym = lookupArray(arrayExpr->name);
        if (arraySym) {
            baseType = arraySym->type;
        }
    } else if (expr.object->getType() == ASTNodeType::EXPR_MEMBER_ACCESS) {
        // Nested member access (e.g., a.b.c)
        baseType = inferMemberAccessType(*static_cast<const MemberAccessExpression*>(expr.object.get()));
    }
    
    // For now, return UNKNOWN - full implementation requires tracking type names in variables
    // This will be completed when we integrate DIM AS TypeName
    return VariableType::UNKNOWN;  // INCOMPLETE!
}
```

**Problems**:
1. Always returns UNKNOWN (no actual implementation!)
2. Cannot look up field types in UDT
3. Cannot handle nested member access properly
4. No error checking for non-existent fields

**Impact**:
- Any code using UDT member access gets UNKNOWN type
- Type checking is effectively disabled for UDT fields
- Codegen must guess at types

---

### 5. Array vs Function Call Ambiguity

**Location**: `fasterbasic_semantic.cpp:2311-2376`

```cpp
VariableType SemanticAnalyzer::inferArrayAccessType(const ArrayAccessExpression& expr) {
    // Mangle the name with its type suffix to match how functions are stored
    std::string mangledName = mangleNameWithSuffix(expr.name, expr.typeSuffix);
    
    // Check if this is a function/sub call first (using mangled name)
    if (m_symbolTable.functions.find(mangledName) != m_symbolTable.functions.end()) {
        // It's a function or sub call - validate arguments but don't treat as array
        const auto& funcSym = m_symbolTable.functions.at(mangledName);
        for (const auto& arg : expr.indices) {
            validateExpression(*arg);
        }
        return funcSym.returnType;
    }
    
    // Otherwise check for existing array (using mangled name)
    if (m_symbolTable.arrays.find(mangledName) != m_symbolTable.arrays.end()) {
        // ... array handling
    }
    
    // Or maybe it's an array without explicit DIM (implicit declaration)
    // ...
}
```

**Problems**:
1. Uses string name mangling to differentiate arrays from functions
2. Parsing ambiguity: `foo(x)` could be array access or function call
3. No clear separation of concerns
4. Error-prone logic with many branches

**Impact**:
- Hard to maintain
- Confusing semantics
- Easy to introduce bugs
- Parser shouldn't need to guess

---

### 6. Separate Variable and Array Symbol Tables

**Location**: `fasterbasic_semantic.h:272-273`

```cpp
struct SymbolTable {
    std::unordered_map<std::string, VariableSymbol> variables;
    std::unordered_map<std::string, ArraySymbol> arrays;
    // ...
};
```

**Problems**:
1. Variables and arrays are in different namespaces
2. Cannot have unified view of all symbols
3. Same name can be both variable and array (confusion!)
4. Lookup logic duplicated

**Impact**:
- Harder to implement proper scoping
- Potential name collisions
- Inefficient lookups (check both tables)
- Conceptually arrays ARE variables with array type

---

### 7. Type Inference Returns Single Enum

**Location**: All `infer*Type()` methods

```cpp
VariableType inferExpressionType(const Expression& expr);
VariableType inferBinaryExpressionType(const BinaryExpression& expr);
VariableType inferArrayAccessType(const ArrayAccessExpression& expr);
// etc.
```

**Problem**:
- Can only return one of ~8 enum values
- Loses all detail about:
  - Array dimensionality
  - UDT identity
  - Const-ness
  - Reference vs value
  - Optional parameters

**Impact**:
- Information loss during type inference
- Cannot reconstruct full type later
- Codegen must re-infer types

---

### 8. No Type Promotion Rules

**Location**: `fasterbasic_semantic.cpp:2545-2567`

```cpp
VariableType SemanticAnalyzer::promoteTypes(VariableType left, VariableType right) {
    // String/Unicode takes precedence
    if (left == VariableType::UNICODE || right == VariableType::UNICODE) {
        return VariableType::UNICODE;
    }
    
    if (left == VariableType::STRING || right == VariableType::STRING) {
        return VariableType::STRING;
    }
    
    // Numeric promotion
    if (left == VariableType::DOUBLE || right == VariableType::DOUBLE) {
        return VariableType::DOUBLE;
    }
    if (left == VariableType::FLOAT || right == VariableType::FLOAT) {
        return VariableType::FLOAT;
    }
    if (left == VariableType::INT || right == VariableType::INT) {
        return VariableType::INT;
    }
    
    return VariableType::FLOAT;  // Default
}
```

**Problems**:
1. No distinction between implicit and explicit conversion
2. No checking if conversion is lossy (DOUBLE -> INT)
3. No handling of UDT types (should error!)
4. No handling of array types

**Example of Problem**:

```basic
DIM x AS INTEGER
DIM y AS DOUBLE

x = y    ' Lossy conversion, should warn or error
y = x    ' Safe conversion, OK
```

Currently: No distinction made

---

### 9. Function Parameter Type Checking is Weak

**Location**: `fasterbasic_semantic.cpp:2408-2444`

```cpp
for (size_t i = 0; i < expr.arguments.size() && i < sym->parameterTypes.size(); ++i) {
    VariableType argType = inferExpressionType(*expr.arguments[i]);
    VariableType paramType = sym->parameterTypes[i];
    std::string paramTypeName = i < sym->parameterTypeNames.size() ? 
                                sym->parameterTypeNames[i] : "";
    
    // Skip validation if parameter type is unknown (untyped parameter)
    if (paramType == VariableType::UNKNOWN && paramTypeName.empty()) {
        continue;  // Skip checking entirely!
    }
    
    // For user-defined types, check type compatibility
    if (paramType == VariableType::USER_DEFINED || argType == VariableType::USER_DEFINED) {
        // Just check that both are USER_DEFINED, don't check WHICH type!
        if (paramType != argType) {
            error(/* ... */);
        }
        // No actual UDT identity checking!
    } else {
        checkTypeCompatibility(paramType, argType, /* ... */);
    }
}
```

**Problems**:
1. Skips checking if parameter type is UNKNOWN
2. USER_DEFINED types just check enum equality, not type identity
3. No checking of array vs scalar
4. No checking of BYREF requirements

**Example of Missed Error**:

```basic
TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

TYPE Sprite
    x AS DOUBLE
    y AS DOUBLE
END TYPE

SUB DrawPoint(p AS Point)
    ' ...
END SUB

DIM s AS Sprite
DrawPoint(s)  ' Should error: Sprite is not Point!
              ' Currently: No error (both are USER_DEFINED)
```

---

### 10. No Array Dimension Validation

**Problem**: Arrays can have any number of dimensions, but we never validate:
- Index count matches dimension count
- Assignment compatibility of arrays

**Example of Missed Errors**:

```basic
DIM arr1(10) AS INTEGER      ' 1D array
DIM arr2(5, 5) AS INTEGER    ' 2D array

arr1 = arr2  ' Should error: incompatible dimensions
x = arr1(1, 2)  ' Should error: too many indices
y = arr2(1)     ' Should error: too few indices
```

Currently: Dimension checking is minimal or absent

---

### 11. Type Information Lost in Codegen

**Location**: QBE codegen files need to reconstruct type information

**Problem**: 
- Semantic analyzer determines types during analysis
- Codegen phase must re-infer types or use string-based lookups
- No clean type information flow from analysis to codegen

**Impact**:
- Duplicate type inference logic
- Potential inconsistencies between phases
- Hard to maintain

---

## Specific Code Quality Issues

### Issue: Magic Strings for Type Names

```cpp
// fasterbasic_semantic.cpp:610-642
if (paramTypeName == "INTEGER" || paramTypeName == "INT") {
    paramType = VariableType::INT;
} else if (paramTypeName == "DOUBLE") {
    paramType = VariableType::DOUBLE;
} else if (paramTypeName == "SINGLE" || paramTypeName == "FLOAT") {
    paramType = VariableType::FLOAT;
} else if (paramTypeName == "STRING") {
    paramType = VariableType::STRING;
}
// ... repeated in multiple places
```

**Better approach**: Central type name registry

---

### Issue: Inconsistent Error Handling

Some checks return early on UNKNOWN:
```cpp
if (expected == VariableType::UNKNOWN || actual == VariableType::UNKNOWN) {
    return;  // Silent pass
}
```

Others treat UNKNOWN as an error:
```cpp
if (type == VariableType::UNKNOWN) {
    error(/* ... */);
}
```

**Impact**: Unpredictable behavior

---

### Issue: Copy-Paste Code

The type name to enum conversion appears in at least 4 places:
- `processFunctionStatement()` for parameters
- `processFunctionStatement()` for return type
- `processSubStatement()` for parameters  
- `processDimStatement()` for array types

**Better approach**: Single helper function

---

## How QBE Exposes These Weaknesses

### QBE Type System

QBE has strong typing for SSA values:
- `w` = word (32-bit)
- `l` = long (64-bit)
- `s` = single (32-bit float)
- `d` = double (64-bit float)

Every instruction's operands must match expected types.

### Examples of QBE Catching Our Errors

**Example 1: Array passed as scalar**

```basic
DIM arr(10) AS INTEGER
x = arr
```

Our codegen might emit:
```qbe
%arr =l alloc8 80        # Array allocation (pointer)
%x =l copy %arr          # Copy pointer - WRONG if x is scalar!
```

QBE will accept this, but it's semantically wrong. Better if we caught it.

**Example 2: Wrong type in function call**

```basic
FUNCTION Foo(x AS DOUBLE) AS INTEGER
    Foo = INT(x)
END FUNCTION

y% = Foo(42%)  ' Pass INTEGER where DOUBLE expected
```

Our codegen might emit:
```qbe
%1 =l copy 42            # INTEGER
%2 =l call $Foo(l %1)    # Call with long (l)
```

But function signature expects:
```qbe
function l $Foo(d %.1)   # Expects double (d)
```

QBE will error: "type mismatch in call". We should catch this in semantic analysis!

---

## Recommendations

### Immediate (Can Do Now)

1. **Add type IDs to TypeSymbol**
   - Give each UDT a unique numeric ID
   - Use ID instead of string comparison

2. **Strengthen checkTypeCompatibility()**
   - Don't skip on UNKNOWN (error instead)
   - Check array vs scalar
   - Check UDT type IDs

3. **Implement inferMemberAccessType()**
   - Look up field types in TypeSymbol
   - Return proper field type

4. **Consolidate type name parsing**
   - Single function to map "INTEGER" -> VariableType::INT
   - Reduces code duplication

### Medium Term (Redesign)

5. **Implement TypeDescriptor system** (see TYPE_SYSTEM_REDESIGN.md)
   - Replaces VariableType enum
   - Can represent arrays, UDTs, etc.

6. **Unify Variable and Array symbol tables**
   - Arrays are just variables with array attribute
   - Single namespace

7. **Add proper conversion tracking**
   - Explicit vs implicit conversions
   - Lossy conversion warnings

### Long Term (Architecture)

8. **Add type annotation to AST nodes**
   - Store inferred type on each Expression node
   - Eliminates need to re-infer during codegen

9. **Implement type inference pass**
   - Separate pass before validation
   - Full bidirectional type inference

10. **Add semantic test suite**
    - Tests for each type checking rule
    - Verify error detection

---

## Metrics: Current Type System Coverage

### Type Checking Coverage (Estimated)

| Check Type | Coverage | Notes |
|------------|----------|-------|
| Scalar type compatibility | 60% | Only string vs numeric |
| Array type checking | 20% | Minimal dimension checks |
| UDT type identity | 30% | Just enum check, no ID |
| Function parameter types | 50% | Basic checking only |
| Member access types | 10% | Returns UNKNOWN |
| Type promotions | 40% | Basic numeric promotion |
| Const-correctness | 0% | Not implemented |
| Reference vs value | 0% | Not tracked |

**Overall Type Safety Score: 30%**

### Lines of Code by Concern

| Module | LOC | % of semantic.cpp |
|--------|-----|-------------------|
| Type inference | ~800 | 21% |
| Type checking | ~200 | 5% |
| Symbol table | ~500 | 13% |
| Statement validation | ~1800 | 48% |
| Control flow | ~300 | 8% |
| Other | ~183 | 5% |

**Observation**: Only 5% of code dedicated to type checking!

---

## Example: Complete Fix for One Issue

### Current Code (Weak)

```cpp
void SemanticAnalyzer::checkTypeCompatibility(VariableType expected, VariableType actual,
                                              const SourceLocation& loc, const std::string& context) {
    if (expected == VariableType::UNKNOWN || actual == VariableType::UNKNOWN) {
        return;  // Skip checking
    }
    
    bool expectedString = (expected == VariableType::STRING || expected == VariableType::UNICODE);
    bool actualString = (actual == VariableType::STRING || actual == VariableType::UNICODE);
    
    if (expectedString != actualString) {
        error(SemanticErrorType::TYPE_MISMATCH,
              "Type mismatch in " + context,
              loc);
    }
}
```

### Proposed Code (Strong)

```cpp
void SemanticAnalyzer::checkTypeCompatibility(const TypeDescriptor& expected, 
                                              const TypeDescriptor& actual,
                                              const SourceLocation& loc, 
                                              const std::string& context) {
    // Error on unknown types (don't silently pass)
    if (expected.baseType == BaseType::UNKNOWN) {
        error(SemanticErrorType::TYPE_ERROR,
              "Cannot determine expected type in " + context,
              loc);
        return;
    }
    
    if (actual.baseType == BaseType::UNKNOWN) {
        error(SemanticErrorType::TYPE_ERROR,
              "Cannot determine actual type in " + context,
              loc);
        return;
    }
    
    // Check array vs scalar
    if (expected.isArray() != actual.isArray()) {
        error(SemanticErrorType::TYPE_MISMATCH,
              "Type mismatch in " + context + ": " +
              (expected.isArray() ? "expected array" : "expected scalar") + ", got " +
              (actual.isArray() ? "array" : "scalar"),
              loc);
        return;
    }
    
    // For arrays, check element type compatibility
    if (expected.isArray()) {
        checkTypeCompatibility(expected.elementType(), actual.elementType(), loc, context + " element");
        return;
    }
    
    // Check UDT type identity
    if (expected.isUserDefined() || actual.isUserDefined()) {
        if (!expected.isUserDefined() || !actual.isUserDefined()) {
            error(SemanticErrorType::TYPE_MISMATCH,
                  "Type mismatch in " + context + ": cannot mix UDT with primitive type",
                  loc);
            return;
        }
        
        if (expected.udtTypeId != actual.udtTypeId) {
            TypeSymbol* expectedType = m_symbolTable.lookupTypeById(expected.udtTypeId);
            TypeSymbol* actualType = m_symbolTable.lookupTypeById(actual.udtTypeId);
            error(SemanticErrorType::TYPE_MISMATCH,
                  "Type mismatch in " + context + ": cannot assign '" +
                  (actualType ? actualType->name : "unknown") + "' to '" +
                  (expectedType ? expectedType->name : "unknown") + "'",
                  loc);
            return;
        }
        return;  // UDT types match
    }
    
    // Check string vs numeric category
    if (expected.isString() != actual.isString()) {
        error(SemanticErrorType::TYPE_MISMATCH,
              "Type mismatch in " + context + ": cannot assign " +
              actual.toString() + " to " + expected.toString(),
              loc);
        return;
    }
    
    // Check numeric precision compatibility
    if (expected.isNumeric() && actual.isNumeric()) {
        // Warn on lossy conversions
        if (actual.baseType == BaseType::DOUBLE && expected.baseType != BaseType::DOUBLE) {
            warning("Possible loss of precision in " + context + 
                   ": converting DOUBLE to " + expected.toString(),
                   loc);
        } else if (actual.baseType == BaseType::SINGLE && expected.baseType == BaseType::INTEGER) {
            warning("Possible loss of precision in " + context + 
                   ": converting SINGLE to INTEGER",
                   loc);
        }
    }
}
```

**Improvements**:
- No silent passes on UNKNOWN
- Checks array vs scalar
- Checks UDT type identity with proper IDs
- Better error messages
- Warnings for lossy conversions

---

## Conclusion

The current semantic analyzer has significant weaknesses in type tracking and checking:

1. **Cannot represent complex types** (arrays, specific UDTs)
2. **Weak type compatibility checking** (many errors slip through)
3. **Incomplete implementations** (member access returns UNKNOWN)
4. **Poor separation of concerns** (arrays vs functions ambiguity)
5. **Information loss** (type details discarded during inference)

These weaknesses are exposed when QBE performs its own type checking on generated SSA, catching errors that should have been caught earlier.

**The solution**: Implement the TypeDescriptor system proposed in `TYPE_SYSTEM_REDESIGN.md` to enable precise, flexible type tracking throughout the compilation pipeline.

**Estimated effort**: 5 weeks for complete migration
**Risk level**: Medium (can be done incrementally)
**Benefit**: High (catches more errors, better codegen, clearer diagnostics)