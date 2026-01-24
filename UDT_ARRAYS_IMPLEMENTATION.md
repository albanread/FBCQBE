# Arrays of User-Defined Types (UDTs) - Implementation

## Overview

This document describes the implementation of arrays of user-defined types (UDTs) in the FasterBASIC → QBE compiler. Arrays of UDTs allow storing collections of structured data efficiently in memory.

## Feature Status

✅ **IMPLEMENTED** - Global/module-level arrays of UDTs with constant dimensions  
⏸️ **PLANNED** - Function-local arrays (two-tier: small on stack, large on heap with defer cleanup)  
⏸️ **PLANNED** - Multi-dimensional UDT arrays  
⏸️ **PLANNED** - Dynamic (runtime-sized) UDT arrays

## Design Decisions

### Memory Allocation Strategy

**Global Arrays (Current Implementation):**
- Allocated on the **heap** using `malloc`
- Allocation happens at program startup (in `main` function's variable declaration section)
- Freed automatically in the exit block of `main`
- Size must be known at compile time

**Why Heap?**
1. Arrays can be arbitrarily large (not constrained by stack size)
2. Consistent with regular array behavior in BASIC
3. Allows for future dynamic sizing support
4. Avoids stack overflow issues

**Future: Function-Local Arrays:**
- **Small arrays** (< threshold, e.g., 1KB): Stack allocation with `alloc8` (fast, automatic cleanup)
- **Large arrays** (≥ threshold): Heap allocation with defer-on-exit cleanup list
- Threshold can be tuned for performance vs. safety

### Element Access

UDT arrays use **direct pointer arithmetic** instead of runtime library calls:
```
element_address = base_pointer + (index * element_size)
```

This provides:
- **Fast access**: No function call overhead
- **Type safety**: Element size computed at compile time
- **Cache efficiency**: Contiguous memory layout

## Implementation Details

### Semantic Analysis (fasterbasic_semantic.cpp)

**Key Change:** When processing `DIM arrayName(size) AS TypeName`:
```cpp
if (arrayDim.hasAsType && !arrayDim.asTypeName.empty()) {
    // Verify type exists
    if (m_symbolTable.types.find(arrayDim.asTypeName) == m_symbolTable.types.end()) {
        error(UNDEFINED_TYPE, ...);
    }
    sym.type = VariableType::USER_DEFINED;
    sym.asTypeName = arrayDim.asTypeName;
}
```

**ArraySymbol Fields:**
- `type`: Set to `USER_DEFINED` for UDT arrays
- `asTypeName`: Stores the UDT type name (e.g., "Point", "RGB")
- `totalSize`: Number of elements (computed from dimensions)

### Code Generation

#### 1. Array Allocation (qbe_codegen_main.cpp)

In `emitMainFunction()`:
```cpp
// Check if UDT array with known dimensions
if (arraySym.type == VariableType::USER_DEFINED && 
    !arraySym.asTypeName.empty() && 
    arraySym.totalSize > 0) {
    
    size_t elementSize = calculateTypeSize(arraySym.asTypeName);
    size_t totalSize = elementSize * arraySym.totalSize;
    
    // Allocate on heap
    std::string sizeTemp = allocTemp("l");
    emit("    " + sizeTemp + " =l copy " + std::to_string(totalSize) + "\n");
    emit("    " + arrayRef + " =l call $malloc(l " + sizeTemp + ")\n");
    
    // Track for element access
    m_arrayElementTypes[name] = arraySym.asTypeName;
}
```

**Generated QBE IL:**
```qbe
%t0 =l copy 48
%arr_points =l call $malloc(l %t0)
# Array points: 3 elements of Point (16 bytes each, heap-allocated)
```

#### 2. DIM Statement Processing (qbe_codegen_statements.cpp)

Skip runtime allocation for UDT arrays (already allocated):
```cpp
void QBECodeGenerator::emitDim(const DimStatement* stmt) {
    for (const auto& arrayDecl : stmt->arrays) {
        // Check if already allocated as UDT array
        if (m_arrayElementTypes.find(arrayName) != m_arrayElementTypes.end()) {
            emitComment("Array " + arrayName + " already allocated on heap");
            continue;
        }
        // ... regular array creation
    }
}
```

#### 3. Array Element Access (qbe_codegen_expressions.cpp)

**For Reading (`points(0).x`):**
```cpp
std::string QBECodeGenerator::emitArrayAccessExpr(const ArrayAccessExpression* expr) {
    auto elemTypeIt = m_arrayElementTypes.find(expr->name);
    if (elemTypeIt != m_arrayElementTypes.end()) {
        // UDT array - pointer arithmetic
        size_t elementSize = calculateTypeSize(typeName);
        
        // index * elementSize
        std::string indexLong = allocTemp("l");
        emit("    " + indexLong + " =l extsw " + indexTemp + "\n");
        
        std::string offsetTemp = allocTemp("l");
        emit("    " + offsetTemp + " =l mul " + indexLong + ", " 
             + std::to_string(elementSize) + "\n");
        
        // base + offset
        std::string elementPtr = allocTemp("l");
        emit("    " + elementPtr + " =l add " + arrayRef + ", " + offsetTemp + "\n");
        
        return elementPtr;  // Returns pointer for member access
    }
    // ... regular array runtime call
}
```

**Generated QBE IL:**
```qbe
%t47 =l extsw %t46
%t48 =l mul %t47, 16          # index * sizeof(Point)
%t49 =l add %arr_points, %t48  # base + offset
%t51 =d loadd %t49             # load .x (offset 0)
```

#### 4. Array Element Assignment (qbe_codegen_statements.cpp)

**For Writing (`points(0).x = 5`):**
```cpp
void QBECodeGenerator::emitLet(const LetStatement* stmt) {
    if (!stmt->memberChain.empty() && !stmt->indices.empty()) {
        // Array element with member access
        auto elemTypeIt = m_arrayElementTypes.find(stmt->variable);
        
        // Compute element address (same as read)
        size_t elementSize = calculateTypeSize(currentTypeName);
        // ... pointer arithmetic ...
        
        // Then walk member chain to final field
        // ... (same as scalar UDT assignment)
    }
}
```

**Generated QBE IL:**
```qbe
%t5 =l extsw %t4
%t6 =l mul %t5, 16            # index * sizeof(Point)
%t7 =l add %arr_points, %t6   # base + offset
%t8 =l copy %t7
stored %t2, %t8                # store to .x (offset 0)
```

#### 5. Cleanup (qbe_codegen_main.cpp)

In `main` exit block:
```cpp
// Free all heap-allocated UDT arrays
for (const auto& [name, arraySym] : m_symbols->arrays) {
    if (arraySym.type == VariableType::USER_DEFINED && 
        !arraySym.asTypeName.empty() && 
        arraySym.totalSize > 0) {
        std::string arrayRef = "%arr_" + name;
        emit("    call $free(l " + arrayRef + ")\n");
    }
}
```

**Generated QBE IL:**
```qbe
@exit
    # Cleanup and return
    call $free(l %arr_colors)
    call $free(l %arr_points)
    call $basic_cleanup()
    ret 0
```

## Usage Example

```basic
TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

TYPE RGB
    r AS INT%
    g AS INT%
    b AS INT%
END TYPE

' Declare arrays
DIM points(2) AS Point
DIM colors(10) AS RGB

' Initialize array elements
points(0).x = 1.5
points(0).y = 2.5

colors(0).r = 255
colors(0).g = 128
colors(0).b = 64

' Read array elements
PRINT "Point 0: ("; points(0).x; ", "; points(0).y; ")"
PRINT "Color RGB("; colors(0).r; ", "; colors(0).g; ", "; colors(0).b; ")"

' Use in loops
FOR i% = 0 TO 2
    points(i%).x = points(i%).x * 2
    PRINT "Point "; i%; ": ("; points(i%).x; ", "; points(i%).y; ")"
NEXT i%
```

## Memory Layout

For `DIM points(2) AS Point` where Point has two DOUBLEs:

```
Base pointer: %arr_points

Offset  Content
------  -------
  0     points(0).x  (8 bytes, DOUBLE)
  8     points(0).y  (8 bytes, DOUBLE)
 16     points(1).x  (8 bytes, DOUBLE)
 24     points(1).y  (8 bytes, DOUBLE)
 32     points(2).x  (8 bytes, DOUBLE)
 40     points(2).y  (8 bytes, DOUBLE)

Total: 48 bytes (3 elements × 16 bytes each)
```

Element access formula: `address = base + (index × 16)`

## Current Limitations

1. **1D Arrays Only**: Multi-dimensional UDT arrays not yet supported
   - Error message emitted if dimensions.size() != 1
   
2. **Constant Dimensions**: Array size must be compile-time constant
   - `DIM arr(10) AS Point` ✅
   - `DIM arr(n%) AS Point` ❌ (where n% is a variable)
   
3. **Global Scope Only**: Arrays in functions not yet implemented
   - Will be added with defer-on-exit cleanup

4. **No Initialization**: Array elements start with undefined values
   - Consider adding zero-initialization option

## QBE Code Characteristics

**Advantages:**
- Simple, straightforward pointer arithmetic
- No runtime overhead for element access
- Type information computed at compile time
- Cache-friendly contiguous layout

**Generated Code Quality:**
- Efficient: Direct memory operations
- Predictable: No hidden allocations
- Debuggable: Clear correspondence to source

## Testing

Test file: `test_udt_arrays.bas`

Tests:
- ✅ Array declaration with AS clause
- ✅ Element assignment (member write)
- ✅ Element reading (member read)
- ✅ Arrays in FOR loops
- ✅ Multiple UDT array types
- ✅ Proper memory allocation and cleanup

## Next Steps

1. **Multi-dimensional arrays**: `DIM grid(10, 10) AS Cell`
   - Row-major layout: `offset = (row * cols + col) * elementSize`
   
2. **Function-local arrays**: Implement defer-on-exit cleanup
   - Track allocations per function scope
   - Emit cleanup before all RET/EXIT paths
   
3. **Dynamic sizing**: `DIM arr(userInput%) AS Point`
   - Evaluate size expression at runtime
   - Pass to malloc
   
4. **Array passing**: Pass arrays to functions/subs
   - By reference (pointer): Fast, default
   - BYVAL option for explicit copy

5. **Initialization syntax**: `DIM arr(2) AS Point = {...}`
   - Optional initializer lists

## Related Documentation

- `UDT_IMPLEMENTATION_STATUS.md` - Scalar UDT implementation
- `UDT_DESIGN_PRINCIPLES.md` - Overall UDT design philosophy
- `TYPE_SYSTEM.md` - Type system and coercion rules