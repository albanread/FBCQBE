# User-Defined Types (UDT) Implementation Plan

## Overview

Implement support for user-defined composite types (structs/records) in the FasterBASIC QBE backend.

**Status**: Parser and semantic analyzer already support TYPE...END TYPE. Need to add QBE code generation.

---

## BASIC Syntax Support

### Type Definition
```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

TYPE Sprite
    Name AS STRING
    Position AS Point    ' Nested type
    Active AS INTEGER
    Health AS DOUBLE
END TYPE
```

### Variable Declaration
```basic
DIM Player AS Sprite
DIM Enemy AS Sprite
```

### Member Access (Read)
```basic
x = Player.Position.X
name$ = Player.Name
```

### Member Assignment (Write)
```basic
Player.Name = "Hero"
Player.Position.X = 100
Player.Position.Y = 200
Player.Health = 100.0
```

### Arrays of UDTs
```basic
DIM Enemies(10) AS Sprite
Enemies(1).Name = "Goblin"
Enemies(1).Position.X = 50
Enemies(1).Health = 30
```

### Multi-dimensional Arrays of UDTs
```basic
DIM Grid(10, 10) AS Point
Grid(5, 5).X = 100
Grid(5, 5).Y = 200
```

---

## Existing Infrastructure

### 1. AST (Already Implemented)
- `TypeDeclarationStatement` - TYPE...END TYPE blocks
- `MemberAccessExpression` - Reading members (e.g., `Player.Position.X`)
- `LetStatement` with `memberChain` - Writing members (e.g., `Player.X = 10`)

### 2. Semantic Analysis (Already Implemented)
- `TypeSymbol` - Stores type definitions with fields
- `SymbolTable.types` - Map of all user-defined types
- Field validation and type checking
- Nested type support

### 3. What's Missing
- **QBE type definitions** (aggregate types)
- **Memory layout calculations** (field offsets, total size)
- **Code generation for member access**
- **Arrays of UDTs**

---

## QBE Representation

### Aggregate Types in QBE

QBE supports aggregate types (structs):

```qbe
# Define types
type :point = { d, d }                        # { X: double, Y: double }
type :sprite = { l, :point, w, d }            # { Name: ptr, Position: point, Active: int, Health: double }

# Allocate on stack
%player =l alloc8 48                           # Allocate sizeof(sprite) bytes
%enemy =l alloc8 48

# Store to member
%health_ptr =l add %player, 32                # offset of Health field
stored d_100.0, %health_ptr

# Load from member
%x_ptr =l add %player, 8                      # offset of Position.X
%x =d loadd %x_ptr
```

### Alternative: Inline Fields (Simpler)

Instead of QBE aggregate types, we can treat UDTs as flattened collections of fields:

```qbe
# Sprite has: Name (l), Position.X (d), Position.Y (d), Active (w), Health (d)
# Just allocate raw memory and track offsets

%player =l alloc8 48                           # sizeof(Sprite)
```

**Decision**: Use **inline memory** approach for simplicity. No QBE aggregate types needed.

---

## Implementation Strategy

### Phase 1: Memory Layout Calculation

**Location**: `qbe_codegen_helpers.cpp`

Add functions:
```cpp
size_t calculateTypeSize(const std::string& typeName);
size_t calculateFieldOffset(const std::string& typeName, const std::string& fieldName);
size_t getFieldOffset(const std::string& typeName, const std::vector<std::string>& memberChain);
```

**Algorithm**:
1. For each field in type:
   - If built-in type: Use fixed sizes (w=4, d=8, l=8)
   - If user-defined: Recursively calculate size
   - Apply alignment (align to 8 bytes for simplicity)
2. Track running offset for each field
3. Cache results in `m_typeSizes` and `m_fieldOffsets` maps

**Sizes**:
- INTEGER (w): 4 bytes
- DOUBLE/FLOAT (d): 8 bytes
- STRING (l): 8 bytes (pointer)
- User-defined: Sum of field sizes (aligned)

**Example**:
```
Point:
  X (d): offset 0, size 8
  Y (d): offset 8, size 8
  Total: 16 bytes

Sprite:
  Name (l): offset 0, size 8
  Position (Point): offset 8, size 16
  Active (w): offset 24, size 4
  Health (d): offset 32, size 8
  Total: 40 bytes (or 48 with alignment)
```

---

### Phase 2: Variable Declaration

**Location**: `qbe_codegen_main.cpp` - `emitMainFunction()`

Modify variable declaration section:

```cpp
if (varSym.type == VariableType::USER_DEFINED) {
    // User-defined type
    size_t typeSize = calculateTypeSize(varSym.typeName);
    std::string varRef = "%var_" + name;
    
    // Allocate memory on stack
    emit("    " + varRef + " =l alloc8 " + std::to_string(typeSize) + "\n");
    
    // Initialize to zero (optional)
    // emit("    call $memset(l " + varRef + ", w 0, l " + std::to_string(typeSize) + ")\n");
    
    m_variables[name] = m_variables.size();
    m_varTypes[name] = "l";  // Pointer type
} else {
    // Existing built-in type handling
    // ...
}
```

---

### Phase 3: Member Access Expression (Read)

**Location**: `qbe_codegen_expressions.cpp`

Add handler for `EXPR_MEMBER_ACCESS`:

```cpp
std::string QBECodeGenerator::emitMemberAccessExpr(const MemberAccessExpression* expr) {
    // 1. Get base object (can be variable or another member access)
    std::string baseTemp = emitExpression(expr->object.get());
    
    // 2. Determine object type
    std::string baseTypeName = inferMemberAccessType(expr->object.get());
    
    // 3. Look up field in type definition
    const TypeSymbol* typeSymbol = m_symbols->types.at(baseTypeName);
    const TypeSymbol::Field* field = typeSymbol->findField(expr->memberName);
    
    if (!field) {
        error("Field not found: " + expr->memberName);
        return allocTemp("w");
    }
    
    // 4. Calculate field offset
    size_t offset = calculateFieldOffset(baseTypeName, expr->memberName);
    
    // 5. Compute member address
    std::string memberPtr = allocTemp("l");
    emit("    " + memberPtr + " =l add " + baseTemp + ", " + std::to_string(offset) + "\n");
    
    // 6. Load value based on field type
    std::string resultTemp;
    if (field->isBuiltIn) {
        std::string qbeType = getQBEType(field->builtInType);
        resultTemp = allocTemp(qbeType);
        
        if (qbeType == "w") {
            emit("    " + resultTemp + " =w loadw " + memberPtr + "\n");
        } else if (qbeType == "d") {
            emit("    " + resultTemp + " =d loadd " + memberPtr + "\n");
        } else if (qbeType == "l") {
            emit("    " + resultTemp + " =l loadl " + memberPtr + "\n");
        }
    } else {
        // Nested UDT - return pointer to nested struct
        resultTemp = memberPtr;
    }
    
    m_stats.instructionsGenerated++;
    return resultTemp;
}
```

**Nested Member Access**: `Player.Position.X`
- First access: `Player.Position` returns pointer to Position
- Second access: `Position.X` loads double from offset

---

### Phase 4: Member Assignment (Write)

**Location**: `qbe_codegen_statements.cpp` - `emitLet()`

Modify to handle member chains:

```cpp
void QBECodeGenerator::emitLet(const LetStatement* stmt) {
    if (!stmt || !stmt->value) return;
    
    // Evaluate right-hand side
    std::string valueTemp = emitExpression(stmt->value.get());
    
    if (!stmt->memberChain.empty()) {
        // Member assignment: Player.Position.X = 100
        
        // 1. Get base variable
        std::string baseRef = getVariableRef(stmt->variable);
        VariableType baseType = getVariableType(stmt->variable);
        
        if (baseType != VariableType::USER_DEFINED) {
            error("Member access on non-UDT variable");
            return;
        }
        
        // 2. Walk member chain to compute final address
        std::string currentPtr = baseRef;
        std::string currentTypeName = getVariableTypeName(stmt->variable);
        
        for (size_t i = 0; i < stmt->memberChain.size(); ++i) {
            const std::string& memberName = stmt->memberChain[i];
            size_t offset = calculateFieldOffset(currentTypeName, memberName);
            
            std::string nextPtr = allocTemp("l");
            emit("    " + nextPtr + " =l add " + currentPtr + ", " + std::to_string(offset) + "\n");
            currentPtr = nextPtr;
            
            // Update type for next iteration (in case of nested UDTs)
            const TypeSymbol* typeSym = m_symbols->types.at(currentTypeName);
            const TypeSymbol::Field* field = typeSym->findField(memberName);
            if (field && !field->isBuiltIn) {
                currentTypeName = field->typeName;
            }
        }
        
        // 3. Get final field type
        const TypeSymbol* finalTypeSym = m_symbols->types.at(currentTypeName);
        const TypeSymbol::Field* finalField = finalTypeSym->findField(stmt->memberChain.back());
        VariableType fieldType = finalField->builtInType;
        
        // 4. Type conversion if needed
        VariableType exprType = inferExpressionType(stmt->value.get());
        if (fieldType != exprType) {
            valueTemp = promoteToType(valueTemp, exprType, fieldType);
        }
        
        // 5. Store value
        std::string qbeType = getQBEType(fieldType);
        if (qbeType == "w") {
            emit("    storew " + valueTemp + ", " + currentPtr + "\n");
        } else if (qbeType == "d") {
            emit("    stored " + valueTemp + ", " + currentPtr + "\n");
        } else if (qbeType == "l") {
            emit("    storel " + valueTemp + ", " + currentPtr + "\n");
        }
        
        m_stats.instructionsGenerated++;
    } else {
        // Existing simple assignment code
        // ...
    }
}
```

---

### Phase 5: Arrays of UDTs

**Location**: `qbe_codegen_statements.cpp` and `qbe_codegen_expressions.cpp`

#### Array Declaration
```cpp
// In emitMainFunction():
if (arraySym.elementType == VariableType::USER_DEFINED) {
    size_t elementSize = calculateTypeSize(arraySym.typeName);
    // Allocate array: numElements * elementSize
}
```

#### Array Element Access with Member
```basic
Enemies(1).Health = 30
```

This is: `Enemies[1]` (get element) + `.Health` (member access)

**Strategy**:
1. Array access returns pointer to element (not value)
2. Member access operates on that pointer

**Implementation**:
```cpp
// Array access for UDT elements
if (elementType == VariableType::USER_DEFINED) {
    size_t elementSize = calculateTypeSize(typeName);
    
    // Calculate address: base + (index * elementSize)
    std::string offsetTemp = allocTemp("l");
    emit("    " + offsetTemp + " =l mul " + indexTemp + ", " + std::to_string(elementSize) + "\n");
    
    std::string elementPtr = allocTemp("l");
    emit("    " + elementPtr + " =l add " + arrayBase + ", " + offsetTemp + "\n");
    
    return elementPtr;  // Return pointer, not value
}
```

Then member access works as normal on the returned pointer.

---

### Phase 6: Type Inference for UDTs

**Location**: `qbe_codegen_helpers.cpp`

Add:
```cpp
std::string QBECodeGenerator::inferMemberAccessType(const Expression* expr) {
    if (expr->getType() == ASTNodeType::EXPR_VARIABLE) {
        const VariableExpression* varExpr = static_cast<const VariableExpression*>(expr);
        auto it = m_symbols->variables.find(varExpr->name);
        if (it != m_symbols->variables.end() && it->second.type == VariableType::USER_DEFINED) {
            return it->second.typeName;
        }
    } else if (expr->getType() == ASTNodeType::EXPR_MEMBER_ACCESS) {
        // Recursively resolve nested member access
        const MemberAccessExpression* memberExpr = static_cast<const MemberAccessExpression*>(expr);
        std::string baseTypeName = inferMemberAccessType(memberExpr->object.get());
        
        const TypeSymbol* typeSym = m_symbols->types.at(baseTypeName);
        const TypeSymbol::Field* field = typeSym->findField(memberExpr->memberName);
        
        if (field && !field->isBuiltIn) {
            return field->typeName;  // Return nested type name
        }
    }
    
    return "";  // Not a UDT
}
```

---

## Implementation Checklist

### Phase 1: Memory Layout (Core Infrastructure)
- [ ] Add `VariableType::USER_DEFINED` enum value
- [ ] Add `calculateTypeSize(typeName)` function
- [ ] Add `calculateFieldOffset(typeName, fieldName)` function
- [ ] Add `getFieldOffset(typeName, memberChain)` for nested access
- [ ] Add caching maps: `m_typeSizes`, `m_fieldOffsets`
- [ ] Handle nested types (recursive calculation)
- [ ] Apply 8-byte alignment

### Phase 2: Variable Declaration
- [ ] Detect user-defined type in variable declaration
- [ ] Emit `alloc8` for UDT variables
- [ ] Store variable as pointer type (`l`)
- [ ] Update `getVariableRef()` to return pointer for UDTs

### Phase 3: Member Access (Read)
- [ ] Add case for `EXPR_MEMBER_ACCESS` in `emitExpression()`
- [ ] Implement `emitMemberAccessExpr()`
- [ ] Handle nested member access (recursive)
- [ ] Emit pointer arithmetic (base + offset)
- [ ] Emit load instructions (`loadw`, `loadd`, `loadl`)

### Phase 4: Member Assignment (Write)
- [ ] Modify `emitLet()` to handle `memberChain`
- [ ] Walk member chain to compute address
- [ ] Handle type conversion for assignment
- [ ] Emit store instructions (`storew`, `stored`, `storel`)

### Phase 5: Arrays of UDTs
- [ ] Modify array declaration for UDT element type
- [ ] Calculate array size: `numElements * sizeof(elementType)`
- [ ] Modify `emitArrayAccessExpr()` for UDT elements
- [ ] Return pointer (not value) for UDT array elements
- [ ] Support multi-dimensional arrays of UDTs

### Phase 6: Type Inference
- [ ] Add `inferMemberAccessType()` function
- [ ] Recursively resolve nested member types
- [ ] Integrate with `inferExpressionType()`

---

## Testing Strategy

### Test 1: Basic Type Definition
```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

DIM P AS Point
P.X = 10.5
P.Y = 20.5
PRINT P.X, P.Y
```

**Expected QBE**:
```qbe
%var_P =l alloc8 16        # sizeof(Point) = 16
%t0 =d copy d_10.5
%t1 =l add %var_P, 0
stored %t0, %t1
%t2 =d copy d_20.5
%t3 =l add %var_P, 8
stored %t2, %t3
%t4 =l add %var_P, 0
%t5 =d loadd %t4
%t6 =l add %var_P, 8
%t7 =d loadd %t6
```

### Test 2: Nested Types
```basic
TYPE Vector2D
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

TYPE Entity
    Name AS STRING
    Position AS Vector2D
    Health AS INTEGER
END TYPE

DIM Player AS Entity
Player.Name = "Hero"
Player.Position.X = 100
Player.Position.Y = 200
Player.Health = 100
```

### Test 3: Arrays of UDTs
```basic
TYPE Sprite
    X AS DOUBLE
    Y AS DOUBLE
    Active AS INTEGER
END TYPE

DIM Sprites(10) AS Sprite
FOR i% = 1 TO 10
    Sprites(i%).X = i% * 10
    Sprites(i%).Y = i% * 20
    Sprites(i%).Active = 1
NEXT i%
```

### Test 4: Multi-dimensional Arrays
```basic
TYPE Cell
    Value AS INTEGER
    Visited AS INTEGER
END TYPE

DIM Grid(8, 8) AS Cell
Grid(4, 4).Value = 42
Grid(4, 4).Visited = 1
```

---

## Edge Cases & Limitations

### Supported
✅ Basic field access  
✅ Nested types  
✅ Arrays of UDTs  
✅ Member assignment  
✅ Mixed built-in and UDT fields  

### Not Supported (Initially)
❌ Passing UDTs to functions (need BYREF/BYVAL semantics)  
❌ Returning UDTs from functions  
❌ UDT comparison (`IF Player = Enemy`)  
❌ Dynamic allocation (`NEW`/`DELETE`)  

### Future Enhancements
- Function parameters: Pass by reference (pointer)
- Function returns: Return pointer or use out-parameter
- Copy operations: Implement `memcpy` for assignment
- Initialization: Support field initializers

---

## QBE Instruction Reference

### Memory Operations
```qbe
# Allocate on stack
%ptr =l alloc8 SIZE          # Allocate SIZE bytes (8-byte aligned)

# Load from memory
%val =w loadw %ptr           # Load 32-bit word
%val =d loadd %ptr           # Load 64-bit double
%val =l loadl %ptr           # Load 64-bit pointer

# Store to memory
storew VALUE, %ptr           # Store 32-bit word
stored VALUE, %ptr           # Store 64-bit double
storel VALUE, %ptr           # Store 64-bit pointer

# Pointer arithmetic
%new_ptr =l add %ptr, OFFSET
```

---

## Implementation Order

1. **Start Simple**: Single-level member access (no nesting)
2. **Add Nesting**: Support `Player.Position.X`
3. **Add Arrays**: Support `Enemies(1).Health`
4. **Add Multi-dim**: Support `Grid(x, y).Value`
5. **Optimize**: Cache offsets, eliminate redundant address calculations

---

## Summary

**Goal**: Full support for user-defined composite types in QBE backend.

**Approach**: 
- Calculate memory layout at compile time
- Use pointer arithmetic for member access
- Treat UDTs as opaque memory blocks (no QBE aggregate types)
- Leverage existing parser/semantic infrastructure

**Status**: Ready to implement!